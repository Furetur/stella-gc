#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <gc.h>
#include <runtime.h>
#include <string.h>

#include "constants.h"

// ------------------------------------
// --- GC State

static bool gc_initialized = false;

static uint8_t *from_space = NULLPTR;
static uint8_t *to_space = NULLPTR;

static uint8_t *allocation_ptr = NULLPTR;
static uint8_t *copy_ptr = NULLPTR;

int gc_roots_next_index = 0;
void **gc_roots[MAX_GC_ROOTS];

void reset_allocation_and_copy_pointers(void) {
  allocation_ptr = from_space;
  copy_ptr = to_space;
}

void initialize_gc_if_needed(void) {
  if (gc_initialized) {
    return;
  }
  if (SPACE_SIZE > MAX_THEORETICAL_SPACE_SIZE) {
    printf("This GC cannot handle %zx bytes...", SPACE_SIZE);
    exit(1);
  }
  uint8_t *total_heap = malloc(2 * MAX_ALLOC_SIZE);
  from_space = (void *)(total_heap);
  to_space = (void *)(total_heap + MAX_ALLOC_SIZE);
  reset_allocation_and_copy_pointers();
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
    return result;
  } else {
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

// TODO: move to runtime
stella_object *copy_object(stella_object *obj) {
  assert(is_in_from_space((uint8_t *)obj));
  assert(is_in_to_space(copy_ptr));
  size_t object_size = size_of_object(obj);
  bool enough_space =
      is_enough_space_for_object(to_space, copy_ptr, object_size);
  assert(enough_space);
  uint8_t *new_location = copy_ptr;
  memcpy(obj, new_location, object_size);
  copy_ptr += object_size;
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
  if (!is_managed_by_gc(obj)) {
    return obj;
  }
  stella_object *new_location = check_if_moved(obj);
  if (new_location != NULLPTR) {
    return new_location;
  }
  new_location = copy_object(obj);
  verify_obj_equal(obj, new_location);
  mark_object(obj, new_location);
  // Visit fields
  int n_fields = STELLA_OBJECT_HEADER_FIELD_COUNT(new_location->object_header);
  for (int i = 0; i < n_fields; i++) {
    stella_object *field = obj->object_fields[i];
    obj->object_fields[i] = recursive_mark_n_copy(field);
  }
  return new_location;
}

void swap_spaces(void) {
  uint8_t *temp = from_space;
  from_space = to_space;
  to_space = temp;
}

void mark_n_copy(void) {
  for (int i = 0; i < gc_roots_next_index; i++) {
    *gc_roots[i] = (void *)recursive_mark_n_copy(*gc_roots[i]);
  }
  swap_spaces();
  reset_allocation_and_copy_pointers();
}

void *gc_alloc(size_t size_in_bytes) {
  initialize_gc_if_needed();
  void *result = try_alloc(size_in_bytes);
  if (result != NULLPTR) {
#ifdef STELLA_GC_DEBUG_MODE
    mark_n_copy();
#endif
    return result;
  }
  mark_n_copy();
  result = try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  printf("Out of memory: could not allocate %zx bytes", size_in_bytes);
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
  if (gc_roots_next_index >= MAX_GC_ROOTS) {
    printf("Out of space for roots: could not push root %p", (void *)ptr);
    exit(1);
  }
  gc_roots[gc_roots_next_index++] = ptr;
}

void gc_pop_root(__attribute__((unused)) void **ptr) {
  initialize_gc_if_needed();
  assert(gc_roots_next_index > 0);
  gc_roots_next_index--;
}
