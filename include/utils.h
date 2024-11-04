#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <stdbool.h>

#include "gc_state.h"
#include <runtime.h>
#include <stdint.h>

size_t size_of_object(stella_object *obj) {
  int fields_count = STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
  return sizeof(stella_object) + fields_count * sizeof(void *);
}

// ------------------------------------
// --- Object location checkers

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

char get_object_location_kind(stella_object *obj) {
  if (!is_managed_by_gc(obj)) {
    return 'U';
  } else if (is_in_from_space((void *)obj)) {
    return 'F';
  } else {
    assert(is_in_to_space((void *)obj));
    return 'T';
  }
}

#endif // UTILS_H
