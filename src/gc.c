#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gc.h>
#include <runtime.h>

#include "constants.h"
#include "gc/debug.h"
#include "gc/forward_pointers.h"
#include "gc/state.h"
#include "gc/utils.h"
#include "runtime_extras.h"

// ------------------------------------
// --- GC State

bool gc_initialized = false;

uint8_t heap[2 * SPACE_SIZE];

uint8_t *fromspace = NULLPTR;
uint8_t *tospace = NULLPTR;

uint8_t *alloc_ptr = NULLPTR;
uint8_t *next_ptr = NULLPTR;
uint8_t *scan_ptr = NULLPTR;

int gc_roots_next_index = 0;
void **gc_roots[MAX_GC_ROOTS];

void initialize_gc_if_needed(void) {
  if (gc_initialized) {
    return;
  }
  uint8_t *total_heap = heap;
  fromspace = (void *)(total_heap);
  tospace = (void *)(total_heap + SPACE_SIZE);
  alloc_ptr = fromspace;
  GC_DEBUG_PRINTF(
      "Initialized GC with SPACE_SIZE=%#zx, from_space=%p, to_space=%p\n",
      SPACE_SIZE, (void *)fromspace, (void *)tospace);
  gc_initialized = true;
}

void *try_alloc(size_t size_in_bytes) {
  bool is_enough_space =
      is_enough_space_left(fromspace, alloc_ptr, size_in_bytes);
  if (is_enough_space) {
    uint8_t *result = alloc_ptr;
    alloc_ptr += size_in_bytes;
    GC_DEBUG_PRINTF("try_alloc: allocated object of size %#zx at %p, new "
                    "alloc_ptr=%p\n",
                    size_in_bytes, (void *)result, (void *)alloc_ptr);
    return result;
  } else {
    GC_DEBUG_PRINTF("try_alloc: not enough space for %#zx bytes\n",
                    size_in_bytes);
    return NULLPTR;
  }
}

stella_object *move_object(stella_object *obj) {
  void *new_location = next_ptr;
  size_t obj_size = copy_object(obj, new_location);
  set_forward_ptr(obj, new_location);
  next_ptr += obj_size;
  GC_DEBUG_PRINTF("move_object(%p): moved to %p, next_ptr=%p\n", (void *)obj,
                  (void *)new_location, (void *)next_ptr);
  GC_DEBUG_PRINT_OBJECT(new_location);
  return new_location;
}

void chase(stella_object *obj) {
  stella_object *current_obj = obj;
  while (current_obj != NULLPTR) {
    GC_DEBUG_PRINTF("chase(%p): chasing %p\n", (void *)obj,
                    (void *)current_obj);
    GC_DEBUG_PRINT_OBJECT(current_obj);
    stella_object *new_location = move_object(current_obj);
    stella_object *last_not_moved_field = NULLPTR;
    for (int i = 0; i < get_fields_count(new_location); i++) {
      stella_object *field = new_location->object_fields[i];
      bool needs_moving =
          points_to_fromspace((void *)field) && !is_forward_ptr(field);
      if (needs_moving) {
        last_not_moved_field = field;
      }
    }
    current_obj = last_not_moved_field;
  }
}

stella_object *forward(stella_object *obj) {
  if (points_to_fromspace((void *)obj)) {
    stella_object *forward_ptr = as_forward_ptr(obj);
    if (forward_ptr != NULLPTR) {
      GC_DEBUG_PRINTF(
          "forward(%p): the object is a forward pointer, return %p\n",
          (void *)obj, (void *)forward_ptr);
      return forward_ptr;
    }
    GC_DEBUG_PRINTF("forward(%p): start chasing\n", (void *)obj);
    chase(obj);
    forward_ptr = as_forward_ptr(obj);
    assert(forward_ptr != NULLPTR);
    assert(points_to_tospace((void *)forward_ptr));
    GC_DEBUG_PRINTF("forward(%p): after chasing return %p\n", (void *)obj,
                    (void *)forward_ptr);
    return forward_ptr;
  } else {
    GC_DEBUG_PRINTF("forward(%p): immediately return %p, because the object is "
                    "not in from-space\n",
                    (void *)obj, (void *)obj);
    return obj;
  }
}

void forward_roots(void) {
  GC_DEBUG_PRINTF("forward_roots(): Forwarding %d roots", gc_roots_next_index);
  for (int i = 0; i < gc_roots_next_index; i++) {
    stella_object **root = (stella_object **)gc_roots[i];
    GC_DEBUG_PRINTF(
        "forward_roots(): Forwarding %d-th root %p which points at object %p\n",
        i, (void *)root, (void *)*root);
    GC_DEBUG_PRINT_OBJECT(*root);
    *root = forward(*root);
  }
}

void forward_fields(stella_object *obj) {
  for (int i = 0; i < get_fields_count(obj); i++) {
    stella_object *field = obj->object_fields[i];
    GC_DEBUG_PRINTF("forward_fields(%p): forwarding %d-th field %p\n",
                    (void *)obj, i, (void *)field);
    stella_object *forwarded_field = forward(field);
    obj->object_fields[i] = forwarded_field;
    GC_DEBUG_PRINTF("forward_fields(%p): Updated %d-th field %p -> %p\n",
                    (void *)obj, i, (void *)field, (void *)forwarded_field);
  }
}

void scan_tospace(void) {
  GC_DEBUG_PRINTF("scan_tospace(): Start scanning: scan_ptr=%p, next_ptr=%p\n",
                  (void *)scan_ptr, (void *)next_ptr);
  while (scan_ptr < next_ptr) {
    stella_object *current_obj = (stella_object *)scan_ptr;
    GC_DEBUG_PRINTF("scan_tospace(): Forwarding fields of object at %p\n",
                    (void *)current_obj);
    GC_DEBUG_PRINT_OBJECT(current_obj);
    forward_fields(current_obj);
    scan_ptr += gc_size_of_object(current_obj);
  }
}

void collect(void) {
  GC_DEBUG_PRINTF(
      ">>>> collect(): Start: fromspace=%p, tospace=%p, alloc_ptr=%p\n",
      (void *)fromspace, (void *)tospace, (void *)alloc_ptr);
  // Prepare
  scan_ptr = tospace;
  next_ptr = tospace;
  // Copy reachable objects
  forward_roots();
  scan_tospace();
  // Swap spaces
  uint8_t *temp = fromspace;
  fromspace = tospace;
  tospace = temp;
  // Set alloc_ptr
  alloc_ptr = next_ptr;
  GC_DEBUG_PRINTF(
      "<<<< collect(): End: fromspace=%p, tospace=%p, alloc_ptr=%p\n",
      (void *)fromspace, (void *)tospace, (void *)alloc_ptr);
}

void *gc_alloc(size_t size_in_bytes) {
  GC_DEBUG_PRINTF("gc_alloc(%#zx)\n", size_in_bytes);
  void *result;
  initialize_gc_if_needed();
#ifdef STELLA_GC_MOVE_ALWAYS
  collect();
#else
  result = try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  collect();
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
  GC_DEBUG_PRINTF("gc_push_root: Pushed root %p\n", (void *)ptr);
  if (gc_roots_next_index >= MAX_GC_ROOTS) {
    printf("Out of space for roots: could not push root %p\n", (void *)ptr);
    exit(1);
  }
  gc_roots[gc_roots_next_index++] = ptr;
}

void gc_pop_root(__attribute__((unused)) void **ptr) {
  initialize_gc_if_needed();
  GC_DEBUG_PRINTF("gc_pop_root: Popped root %p\n", (void *)ptr);
  assert(gc_roots_next_index > 0);
  gc_roots_next_index--;
}
