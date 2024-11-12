#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <runtime.h>

#include "gc/gen1.h"

#include "constants.h"
#include "gc/debug.h"
#include "gc/forward_pointers.h"
#include "gc/parameters.h"
#include "gc/roots.h"
#include "gc/stats.h"
#include "gc/utils.h"
#include "runtime_extras.h"

// ------------------------------------
// --- GC State

bool gen1_gc_initialized = false;

uint8_t *gen1_fromspace = NULLPTR;
uint8_t *gen1_tospace = NULLPTR;

uint8_t *gen1_alloc_ptr = NULLPTR;
uint8_t *gen1_next_ptr = NULLPTR;
uint8_t *gen1_scan_ptr = NULLPTR;

void gen1_initialize(void) {
  assert(!gen1_gc_initialized);
  uint8_t *total_heap = malloc(2 * GEN1_SPACE_SIZE);
  gen1_fromspace = (void *)(total_heap);
  gen1_tospace = (void *)(total_heap + GEN1_SPACE_SIZE);
  gen1_alloc_ptr = gen1_fromspace;
  GC_DEBUG_PRINTF(
      "Initialized GC with SPACE_SIZE=%#zx, from_space=%p, to_space=%p\n",
      GEN1_SPACE_SIZE, (void *)gen1_fromspace, (void *)gen1_tospace);
  gen1_gc_initialized = true;
}

static void *gen1_try_alloc(size_t size_in_bytes) {
  return try_alloc(gen1_fromspace, GEN1_SPACE_SIZE, &gen1_alloc_ptr,
                   size_in_bytes);
}

static stella_object *move_object(stella_object *obj) {
  void *new_location = gen1_next_ptr;
  size_t obj_size = copy_object(obj, new_location);
  set_forward_ptr(obj, new_location);
  gen1_next_ptr += obj_size;
  GC_DEBUG_PRINTF("move_object(%p): moved to %p, next_ptr=%p\n", (void *)obj,
                  (void *)new_location, (void *)gen1_next_ptr);
  GC_DEBUG_PRINT_OBJECT(new_location);
  return new_location;
}

static void chase(stella_object *obj) {
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

static stella_object *gen1_forward(stella_object *obj) {
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
    GC_DEBUG_PRINTF("forward(%p): finished chasing, return %p\n", (void *)obj,
                    (void *)forward_ptr);
    return forward_ptr;
  } else {
    GC_DEBUG_PRINTF("forward(%p): immediately return %p, because the object is "
                    "not in from-space\n",
                    (void *)obj, (void *)obj);
    return obj;
  }
}

static void gen1_forward_var_roots(void) {
  GC_DEBUG_PRINTF("gen1_forward_var_roots(): Forwarding %d roots\n",
                  var_roots_next_index);
  for (int i = 0; i < var_roots_next_index; i++) {
    stella_object **root = (stella_object **)var_roots[i];
    GC_DEBUG_PRINTF("gen1_forward_var_roots(): Forwarding %d-th root %p which "
                    "points at object %p\n",
                    i, (void *)root, (void *)*root);
    GC_DEBUG_PRINT_OBJECT(*root);
    *root = gen1_forward(*root);
  }
}

static void gen1_forward_roots_from_gen0(void) {
  scan_gen0_for_roots_to_gen1();
  for (int i = 0; i < roots_from_gen0_to_gen1_next_index; i++) {
    stella_object **root = (stella_object **)roots_from_gen0_to_gen1[i];
    GC_DEBUG_PRINTF("gen1_forward_roots_from_gen0(): Forwarding %d-th root %p "
                    "which points at object %p\n",
                    i, (void *)root, (void *)*root);
    GC_DEBUG_PRINT_OBJECT(*root);
    *root = gen1_forward(*root);
  }
}

static void forward_fields(stella_object *obj) {
  for (int i = 0; i < get_fields_count(obj); i++) {
    stella_object *field = obj->object_fields[i];
    GC_DEBUG_PRINTF("forward_fields(%p): forwarding %d-th field %p\n",
                    (void *)obj, i, (void *)field);
    stella_object *forwarded_field = gen1_forward(field);
    obj->object_fields[i] = forwarded_field;
    GC_DEBUG_PRINTF("forward_fields(%p): Updated %d-th field %p -> %p\n",
                    (void *)obj, i, (void *)field, (void *)forwarded_field);
  }
}

static void scan_tospace(void) {
  GC_DEBUG_PRINTF("scan_tospace(): Start scanning: scan_ptr=%p, next_ptr=%p\n",
                  (void *)gen1_scan_ptr, (void *)gen1_next_ptr);
  while (gen1_scan_ptr < gen1_next_ptr) {
    stella_object *current_obj = (stella_object *)gen1_scan_ptr;
    GC_DEBUG_PRINTF("scan_tospace(): Forwarding fields of object at %p\n",
                    (void *)current_obj);
    GC_DEBUG_PRINT_OBJECT(current_obj);
    forward_fields(current_obj);
    gen1_scan_ptr += gc_size_of_object(current_obj);
  }
}

void gen1_collect(void) {
  GC_DEBUG_PRINTF(
      ">>>> gen1_collect(): Start: fromspace=%p, tospace=%p, alloc_ptr=%p\n",
      (void *)gen1_fromspace, (void *)gen1_tospace, (void *)gen1_alloc_ptr);
  stats_record_collect();
  // Prepare
  gen1_scan_ptr = gen1_tospace;
  gen1_next_ptr = gen1_tospace;
  // Copy reachable objects
  gen1_forward_var_roots();
  gen1_forward_roots_from_gen0();
  scan_tospace();
  // Swap spaces
  uint8_t *temp = gen1_fromspace;
  gen1_fromspace = gen1_tospace;
  gen1_tospace = temp;
  // Set alloc_ptr
  gen1_alloc_ptr = gen1_next_ptr;
  GC_DEBUG_PRINTF(
      "<<<< gen1_collect(): End: fromspace=%p, tospace=%p, alloc_ptr=%p\n",
      (void *)gen1_fromspace, (void *)gen1_tospace, (void *)gen1_alloc_ptr);
}

void *gen1_alloc(size_t size_in_bytes) {
  GC_DEBUG_PRINTF("gen1_gc_alloc(%#zx)\n", size_in_bytes);
  void *result;
#ifdef STELLA_GC_MOVE_ALWAYS
  GC_DEBUG_PRINTF("gen1_gc_alloc(%#zx): Starting collection because "
                  "STELLA_GC_MOVE_ALWAYS=ON\n",
                  size_in_bytes);
  gen1_collect();
#else
  result = gen1_try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  GC_DEBUG_PRINTF(
      "gen1_gc_alloc(%#zx): Starting collection because there is not "
      "enough space for object\n",
      size_in_bytes);
  collect();
#endif
  result = gen1_try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  printf("Out of memory: could not allocate %zx bytes in Gen1\n",
         size_in_bytes);
  exit(1);
}
