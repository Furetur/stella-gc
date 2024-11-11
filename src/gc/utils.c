#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <runtime.h>

#include "gc/utils.h"

#include "gc/gen1.h"
#include "gc/parameters.h"
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

bool points_to_fromspace(uint8_t *ptr) {
  return points_to_some_space(gen1_fromspace, ptr, GEN1_SPACE_SIZE);
}

bool points_to_tospace(uint8_t *ptr) {
  return points_to_some_space(gen1_tospace, ptr, GEN1_SPACE_SIZE);
}

bool is_managed_by_gc(stella_object *obj) {
  uint8_t *ptr = (uint8_t *)obj;
  return points_to_tospace(ptr) || points_to_fromspace(ptr);
}

bool is_enough_space_left(uint8_t *space, uint8_t *location_ptr,
                          size_t size_in_bytes) {
  assert(space <= location_ptr);
  assert(location_ptr <= space + GEN1_SPACE_SIZE);
  return (location_ptr + size_in_bytes) <= (space + GEN1_SPACE_SIZE);
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
