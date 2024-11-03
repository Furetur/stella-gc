#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <gc.h>
#include <runtime.h>
#include <string.h>

#include "constants.h"

#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINTF(fmt, ...) printf("[debug gc] " fmt, __VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#endif

// ------------------------------------
// --- GC State

static bool gc_initialized = false;

static uint8_t *from_space = NULLPTR;
static uint8_t *to_space = NULLPTR;

static uint8_t *allocation_ptr = NULLPTR;
static uint8_t *copy_ptr = NULLPTR;

int gc_roots_next_index = 0;
void **gc_roots[MAX_GC_ROOTS];

void initialize_gc_if_needed(void) {
  if (gc_initialized) {
    return;
  }
  if (SPACE_SIZE > MAX_THEORETICAL_SPACE_SIZE) {
    printf("This GC cannot handle %zx bytes...\n", SPACE_SIZE);
    exit(1);
  }
  uint8_t *total_heap = malloc(2 * SPACE_SIZE);
  from_space = (void *)(total_heap);
  to_space = (void *)(total_heap + SPACE_SIZE);
  allocation_ptr = from_space;
  copy_ptr = to_space;
  DEBUG_PRINTF(
      "Initialized GC with SPACE_SIZE=%#zx, from_space=%p, to_space=%p\n",
      SPACE_SIZE, (void *)from_space, (void *)to_space);
  gc_initialized = true;
}

bool is_in_space(uint8_t *space, uint8_t *ptr) {
  return (space <= ptr) && (ptr < (space + SPACE_SIZE));
}

bool is_in_from_space(uint8_t *ptr) { return is_in_space(from_space, ptr); }

bool is_in_to_space(uint8_t *ptr) { return is_in_space(to_space, ptr); }

bool is_enough_space_for_object(uint8_t *space, uint8_t *location_ptr,
                                size_t size_in_bytes) {
  assert(space <= location_ptr);
  assert(location_ptr <= space + SPACE_SIZE);
  return (location_ptr + size_in_bytes) <= (space + SPACE_SIZE);
}

bool is_managed_by_gc(stella_object *obj) {
  uint8_t *ptr = (uint8_t *)obj;
  return is_in_to_space(ptr) || is_in_from_space(ptr);
}

void *try_alloc(size_t size_in_bytes) {
  bool is_enough_space =
      is_enough_space_for_object(from_space, allocation_ptr, size_in_bytes);
  if (is_enough_space) {
    uint8_t *result = allocation_ptr;
    allocation_ptr += size_in_bytes;
    DEBUG_PRINTF("try_alloc: allocated object of size %#zx at %p, new "
                 "allocation_ptr=%p\n",
                 size_in_bytes, (void *)result, (void *)allocation_ptr);
    return result;
  } else {
    DEBUG_PRINTF("try_alloc: not enough space for %#zx bytes\n", size_in_bytes);
    return NULLPTR;
  }
}

void mark_object(stella_object *obj, stella_object *new_obj_location) {
  assert(is_in_from_space((uint8_t *)obj));
  assert(is_in_to_space((uint8_t *)new_obj_location));
  uint64_t new_location_offset = ((uint8_t *)new_obj_location) - to_space;
  assert(new_location_offset <= UINT32_MAX);
  uint32_t compressed_new_location_offset = (uint32_t)new_location_offset;
  STELLA_OBJECT_INIT_TAG(obj, TAG_MOVED);
  STELLA_OBJECT_INIT_FIELDS_COUNT(obj, compressed_new_location_offset);
}

stella_object *check_if_moved(stella_object *obj) {
  if (is_in_to_space((uint8_t *)obj)) {
    return obj;
  }
  int header = obj->object_header;
  int tag = STELLA_OBJECT_HEADER_TAG(header);
  if (tag != TAG_MOVED) {
    return NULLPTR;
  }
  int new_location_offset = STELLA_OBJECT_HEADER_FIELD_COUNT(header);
  uint8_t *new_location = (to_space + new_location_offset);
  return (stella_object *)new_location;
}

// TODO: move to runtime
size_t size_of_object(stella_object *obj) {
  return (1 + STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header)) *
         sizeof(void *);
}

bool is_a_forward_pointer(stella_object *obj) {
  stella_object *new_location = check_if_moved(obj);
  return new_location != NULLPTR;
}

void debug_print_object(stella_object *obj) {
  printf("[debug gc]\t\tStella object at %p: ", (void *)obj);
  stella_object *new_location = check_if_moved(obj);
  if (new_location != NULLPTR) {
    printf("FORWARD POINTER TO %p", (void *)new_location);
  } else {
    size_t size = size_of_object(obj);
    printf("(size %#zx) ", size);
    print_stella_object(obj);
    printf("\n");
  }
}

#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINT_OBJECT(obj) debug_print_object(obj)
#else
#define DEBUG_PRINT_OBJECT(obj) ((void)0)
#endif

// TODO: move to runtime
stella_object *copy_object(stella_object *obj) {
  assert(is_in_from_space((uint8_t *)obj));
  assert(is_in_to_space(copy_ptr));
  assert(!is_a_forward_pointer(obj));
  DEBUG_PRINTF("copy_object(%p): enter\n", (void *)obj);
  DEBUG_PRINT_OBJECT(obj);
  size_t object_size = size_of_object(obj);
  bool enough_space =
      is_enough_space_for_object(to_space, copy_ptr, object_size);
  assert(enough_space);
  uint8_t *new_location = copy_ptr;
  memcpy(new_location, obj, object_size);
  copy_ptr += object_size;
  DEBUG_PRINTF("copy_object(%p): copied to %p, new copy_ptr=%p\n", (void *)obj,
               (void *)new_location, (void *)copy_ptr);
  DEBUG_PRINT_OBJECT((stella_object *)new_location);
  return (stella_object *)new_location;
}

void verify_obj_equal(stella_object *obj1, stella_object *obj2) {
  assert(obj1->object_header == obj2->object_header);
  int n_fields = STELLA_OBJECT_HEADER_FIELD_COUNT(obj1->object_header);
  for (int i = 0; i < n_fields; i++) {
    assert(obj1->object_fields[i] == obj2->object_fields[i]);
  }
}

stella_object *recursive_mark_n_copy(stella_object *obj) {
  DEBUG_PRINTF(">>>> >>>> recursive_mark_n_copy(%p): start\n", (void *)obj);
  if (!is_managed_by_gc(obj)) {
    DEBUG_PRINTF("<<<< <<<< recursive_mark_n_copy(%p): not managed by GC\n",
                 (void *)obj);
    return obj;
  }
  DEBUG_PRINT_OBJECT(obj);
  stella_object *new_location = check_if_moved(obj);
  if (new_location != NULLPTR) {
    DEBUG_PRINTF("<<<< <<<< recursive_mark_n_copy(%p): already moved to %p\n",
                 (void *)obj, (void *)new_location);
    return new_location;
  }
  new_location = copy_object(obj);
  verify_obj_equal(obj, new_location);
  mark_object(obj, new_location);
  // Visit fields
  int n_fields = STELLA_OBJECT_HEADER_FIELD_COUNT(new_location->object_header);
  for (int i = 0; i < n_fields; i++) {
    stella_object *field = new_location->object_fields[i];
    DEBUG_PRINTF("recursive_mark_n_copy(%p): visiting %d-th field %p\n",
                 (void *)obj, i, (void *)field);
    stella_object *forwarded_field = recursive_mark_n_copy(field);
    new_location->object_fields[i] = forwarded_field;
    DEBUG_PRINTF(
        "recursive_mark_n_copy(%p): updated %d-th field from %p --> to %p\n",
        (void *)obj, i, (void *)field, (void *)forwarded_field);
  }
  DEBUG_PRINTF("<<<< <<<< recursive_mark_n_copy(%p): moved to %p\n",
               (void *)obj, (void *)new_location);
  return new_location;
}

void swap_spaces(void) {
  // Swap space pointers
  uint8_t *temp = from_space;
  from_space = to_space;
  to_space = temp;
  // Reset allocation and copy pointers
  allocation_ptr = copy_ptr;
  copy_ptr = to_space;
}

void mark_n_copy(void) {
  DEBUG_PRINTF(">>>> Start mark_n_copy: from_space=%p, to_space=%p, "
               "allocation_ptr=%p, copy_ptr=%p\n",
               (void *)from_space, (void *)to_space, (void *)allocation_ptr,
               (void *)copy_ptr);
  for (int i = 0; i < gc_roots_next_index; i++) {
    DEBUG_PRINTF("mark_n_copy: visiting root %p that points to object %p\n",
                 (void *)gc_roots[i], *gc_roots[i]);
    void *forwarded_root = recursive_mark_n_copy(*gc_roots[i]);
    DEBUG_PRINTF("mark_n_copy: updated root %p: from %p --> to %p\n",
                 (void *)gc_roots[i], *gc_roots[i], forwarded_root);
    *gc_roots[i] = forwarded_root;
  }
  swap_spaces();
  DEBUG_PRINTF("<<<< End mark_n_copy: from_space=%p, to_space=%p, "
               "allocation_ptr=%p, copy_ptr=%p\n",
               (void *)from_space, (void *)to_space, (void *)allocation_ptr,
               (void *)copy_ptr);
}

void *gc_alloc(size_t size_in_bytes) {
  void *result;
  DEBUG_PRINTF("gc_alloc(%#zx)\n", size_in_bytes);
  initialize_gc_if_needed();
#ifdef STELLA_GC_DEBUG_MODE
  mark_n_copy();
#else
  result = try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  mark_n_copy();
#endif
  result = try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  printf("Out of memory: could not allocate %zx bytes\n", size_in_bytes);
  exit(1);
}

void print_gc_roots(void) { initialize_gc_if_needed(); }

void print_gc_alloc_stats(void) { initialize_gc_if_needed(); }

void print_gc_state(void) {
  initialize_gc_if_needed();
  // TODO: not implemented
}

void gc_read_barrier(__attribute__((unused)) void *object,
                     __attribute__((unused)) int field_index) {
  initialize_gc_if_needed();
}

void gc_write_barrier(__attribute__((unused)) void *object,
                      __attribute__((unused)) int field_index,
                      __attribute__((unused)) void *contents) {
  initialize_gc_if_needed();
}

void gc_push_root(void **ptr) {
  initialize_gc_if_needed();
  DEBUG_PRINTF("gc_push_root: Pushed root %p that points to object %p\n",
               (void *)ptr, *ptr);
  if (gc_roots_next_index >= MAX_GC_ROOTS) {
    printf("Out of space for roots: could not push root %p\n", (void *)ptr);
    exit(1);
  }
  gc_roots[gc_roots_next_index++] = ptr;
}

void gc_pop_root(__attribute__((unused)) void **ptr) {
  initialize_gc_if_needed();
  DEBUG_PRINTF("gc_pop_root: Popped root %p that points to object %p\n",
               (void *)ptr, *ptr);
  assert(gc_roots_next_index > 0);
  gc_roots_next_index--;
}
