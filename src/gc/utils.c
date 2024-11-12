#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <runtime.h>

#include "constants.h"
#include "gc/debug.h"
#include "gc/gen0.h"
#include "gc/gen1.h"
#include "gc/parameters.h"
#include "gc/stats.h"
#include "gc/utils.h"
#include "runtime_extras.h"

// Size of stella object
size_t gc_size_of_object(stella_object *obj) {
  int fields_count = STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
  return (1 + fields_count) * sizeof(void *);
}

// ------------------------------------
// --- Object location checkers

bool points_to_some_space(uint8_t *space, uint8_t *ptr, size_t space_size) {
  return (space <= ptr) && (ptr < (space + space_size));
}

bool points_to_gen0_space(uint8_t *ptr) {
  return points_to_some_space(gen0_space, ptr, GEN0_SPACE_SIZE);
}

bool points_to_fromspace(uint8_t *ptr) {
  return points_to_some_space(gen1_fromspace, ptr, GEN1_SPACE_SIZE);
}

bool points_to_tospace(uint8_t *ptr) {
  return points_to_some_space(gen1_tospace, ptr, GEN1_SPACE_SIZE);
}

bool is_managed_by_gc(stella_object *obj) {
  uint8_t *ptr = (uint8_t *)obj;
  return points_to_gen0_space(ptr) || points_to_tospace(ptr) ||
         points_to_fromspace(ptr);
}

bool is_enough_space_left_for_object(uint8_t *space_start, size_t space_size,
                                     uint8_t *dest, size_t object_size) {
  assert(space_start <= dest);
  assert(dest <= space_start + GEN1_SPACE_SIZE);
  return (dest + object_size) <= (space_start + space_size);
}

void *try_alloc(uint8_t *space_start, size_t space_size, uint8_t **alloc_ptr,
                size_t size_in_bytes) {
  bool can_allocate = is_enough_space_left_for_object(
      space_start, space_size, *alloc_ptr, size_in_bytes);
  if (can_allocate) {
    stats_record_allocation(size_in_bytes);
    uint8_t *result = *alloc_ptr;
    *alloc_ptr = (*alloc_ptr) + size_in_bytes;
    GC_DEBUG_PRINTF("try_alloc: allocated object of size %#zx at %p, new "
                    "alloc_ptr=%p\n",
                    size_in_bytes, (void *)result, (void *)*alloc_ptr);
    stats_record_max_residency();
    return result;
  } else {
    GC_DEBUG_PRINTF("try_alloc: not enough space for %#zx bytes\n",
                    size_in_bytes);
    return NULLPTR;
  }
}

// ------------------------------------
// --- Copy Objects

void assert_objects_equal(stella_object *obj1, stella_object *obj2) {
  // Check headers
  assert(get_fields_count(obj1) == get_fields_count(obj2));
  assert(get_tag(obj1) == get_tag(obj2));
  assert(obj1->object_header == obj2->object_header);
  // Check sizes
  assert(gc_size_of_object(obj1) == gc_size_of_object(obj2));
  // Check fields
  int n_fields = get_fields_count(obj1);
  for (int i = 0; i < n_fields; i++) {
    assert(obj1->object_fields[i] == obj2->object_fields[i]);
  }
}

size_t copy_object(stella_object *obj, void *dest) {
  size_t size = gc_size_of_object(obj);
  memcpy(dest, obj, size);
  // Verify
  assert_objects_equal(obj, (stella_object *)dest);
  return size;
}
