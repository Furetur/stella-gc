#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "gc/forward_pointers.h"
#include "gc/gen0.h"

#include "constants.h"
#include "gc/debug.h"
#include "gc/gen1.h"
#include "gc/parameters.h"
#include "gc/roots.h"
#include "gc/utils.h"
#include "runtime.h"
#include "runtime_extras.h"

bool gen0_gc_initialized = false;

uint8_t *gen0_space = NULLPTR;

uint8_t *gen0_alloc_ptr = NULLPTR;
uint8_t *gen0_scan_ptr = NULLPTR;

void gen0_initialize(void) {
  assert(!gen0_gc_initialized);
  gen0_space = malloc(GEN0_SPACE_SIZE);
  gen0_alloc_ptr = gen0_space;
  gen0_gc_initialized = true;
}

void *gen0_try_alloc(size_t size_in_bytes) {
  return try_alloc(gen0_space, GEN0_SPACE_SIZE, &gen0_alloc_ptr, size_in_bytes);
}

static stella_object *move_object_to_gen1(stella_object *obj) {
  size_t size = gc_size_of_object(obj);
  void *new_location = gen1_alloc(size);
  copy_object(obj, new_location);
  set_forward_ptr(obj, new_location);
  GC_DEBUG_PRINTF("move_object_to_gen1(%p): moved to %p\n", (void *)obj,
                  (void *)new_location);
  GC_DEBUG_PRINT_OBJECT(new_location);
  return new_location;
}

static void gen0_chase(stella_object *obj) {
  stella_object *current_obj = obj;
  while (current_obj != NULLPTR) {
    GC_DEBUG_PRINTF("gen0_chase(%p): chasing %p\n", (void *)obj,
                    (void *)current_obj);
    GC_DEBUG_PRINT_OBJECT(current_obj);
    stella_object *new_location = move_object_to_gen1(current_obj);
    stella_object *last_not_moved_field = NULLPTR;
    for (int i = 0; i < get_fields_count(new_location); i++) {
      stella_object *field = new_location->object_fields[i];
      bool needs_moving =
          points_to_gen0_space((void *)field) && !is_forward_ptr(field);
      if (needs_moving) {
        last_not_moved_field = field;
      }
    }
    current_obj = last_not_moved_field;
  }
}

static stella_object *gen0_forward(stella_object *obj) {
  if (points_to_gen0_space((void *)obj)) {
    stella_object *forward_ptr = as_forward_ptr(obj);
    if (forward_ptr != NULLPTR) {
      GC_DEBUG_PRINTF(
          "gen0_forward(%p): the object is a forward pointer, return %p\n",
          (void *)obj, (void *)forward_ptr);
      return forward_ptr;
    }
    GC_DEBUG_PRINTF("gen0_forward(%p): start chasing\n", (void *)obj);
    gen0_chase(obj);
    forward_ptr = as_forward_ptr(obj);
    assert(forward_ptr != NULLPTR);
    assert(points_to_fromspace((void *)forward_ptr));
    GC_DEBUG_PRINTF("gen0_forward(%p): finished chasing, return %p\n",
                    (void *)obj, (void *)forward_ptr);
    return forward_ptr;
  } else {
    GC_DEBUG_PRINTF(
        "gen0_forward(%p): immediately return %p, because the object is "
        "not in from-space\n",
        (void *)obj, (void *)obj);
    return obj;
  }
}

static void gen0_forward_var_roots(void) {
  GC_DEBUG_PRINTF("gen0_forward_roots(): Forwarding %d roots\n",
                  var_roots_next_index);
  for (int i = 0; i < var_roots_next_index; i++) {
    stella_object **root = (stella_object **)var_roots[i];
    GC_DEBUG_PRINTF(
        "forward_roots(): Forwarding %d-th root %p which points at object %p\n",
        i, (void *)root, (void *)*root);
    GC_DEBUG_PRINT_OBJECT(*root);
    *root = gen0_forward(*root);
  }
}

static void gen0_forward_roots_from_gen1(void) {
  scan_gen1_for_roots_to_gen0();
  GC_DEBUG_PRINTF("gen0_forward_roots_from_gen1(): Forwarding %d roots\n",
                  roots_from_gen1_to_gen0_next_index);
  for (int i = 0; i < roots_from_gen1_to_gen0_next_index; i++) {
    stella_object **root = (stella_object **)roots_from_gen1_to_gen0[i];
    GC_DEBUG_PRINTF("gen0_forward_roots_from_gen1(): Forwarding %d-th root %p "
                    "which points at object %p\n",
                    i, (void *)root, (void *)*root);
    GC_DEBUG_PRINT_OBJECT(*root);
    *root = gen0_forward(*root);
  }
}

static void gen0_forward_fields(stella_object *obj) {
  for (int i = 0; i < get_fields_count(obj); i++) {
    stella_object *field = obj->object_fields[i];
    GC_DEBUG_PRINTF("gen0_forward_fields(%p): forwarding %d-th field %p\n",
                    (void *)obj, i, (void *)field);
    stella_object *forwarded_field = gen0_forward(field);
    obj->object_fields[i] = forwarded_field;
    GC_DEBUG_PRINTF("gen0_forward_fields(%p): Updated %d-th field %p -> %p\n",
                    (void *)obj, i, (void *)field, (void *)forwarded_field);
  }
}

static void gen0_scan(void) {
  GC_DEBUG_PRINTF(
      "gen0_scan_gen1(): Start scanning: gen0_scan_ptr=%p, gen1_alloc_ptr=%p\n",
      (void *)gen0_scan_ptr, (void *)gen1_alloc_ptr);
  while (gen0_scan_ptr < gen1_alloc_ptr) {
    stella_object *current_obj = (stella_object *)gen0_scan_ptr;
    GC_DEBUG_PRINTF("gen0_scan_gen1(): Forwarding fields of object at %p\n",
                    (void *)current_obj);
    GC_DEBUG_PRINT_OBJECT(current_obj);
    gen0_forward_fields(current_obj);
    gen0_scan_ptr += gc_size_of_object(current_obj);
  }
}

void gen0_collect(void) {
  GC_DEBUG_PRINTF(
      ">>>> gen0_collect(): Start: gen0_space=%p, gen1_alloc_ptr=%p\n",
      (void *)gen0_space, (void *)gen1_alloc_ptr);
  // Prepare
  gen1_scan_ptr = gen1_alloc_ptr;
  // Copy reachable objects
  gen0_forward_var_roots();
  gen0_forward_roots_from_gen1();
  gen0_scan();
  GC_DEBUG_PRINTF(
      "<<<< gen0_collect(): End: gen0_space=%p, gen1_alloc_ptr=%p\n",
      (void *)gen0_space, (void *)gen1_alloc_ptr);
}

void *gen0_alloc(size_t size_in_bytes) {
  void *result = gen0_try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  gen0_collect();
  result = gen0_try_alloc(size_in_bytes);
  if (result != NULLPTR) {
    return result;
  }
  printf("Out of memory: could not allocate %zx bytes in Gen0\n",
         size_in_bytes);
  exit(1);
}
