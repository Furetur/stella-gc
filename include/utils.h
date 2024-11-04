#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <assert.h>

#include <runtime.h>
#include "gc_state.h"

size_t size_of_object(stella_object *obj) {
  return (1 + STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header)) *
         sizeof(void *);
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
    } else if (is_in_from_space((void*)obj)) {
        return 'F';
    } else {
        assert(is_in_to_space((void*) obj));
        return 'T';
    }
}

// ------------------------------------
// --- Forward pointers

void set_forward_pointer(stella_object *obj, stella_object *new_obj_location) {
  assert(is_in_from_space((uint8_t *)obj));
  assert(is_in_to_space((uint8_t *)new_obj_location));
  uint64_t new_location_offset = ((uint8_t *)new_obj_location) - to_space;
  assert(new_location_offset <= UINT32_MAX);
  uint32_t compressed_new_location_offset = (uint32_t)new_location_offset;
  STELLA_OBJECT_INIT_TAG(obj, TAG_MOVED);
  STELLA_OBJECT_INIT_FIELDS_COUNT(obj, compressed_new_location_offset);
}

bool is_a_forward_pointer(stella_object *obj);

stella_object *check_if_is_a_forward_pointer(stella_object *obj) {
  int header = obj->object_header;
  int tag = STELLA_OBJECT_HEADER_TAG(header);
  if (tag != TAG_MOVED) {
    return NULLPTR;
  }
  int new_location_offset = STELLA_OBJECT_HEADER_FIELD_COUNT(header);
  uint8_t *new_location = (to_space + new_location_offset);
  assert(is_in_to_space(new_location));
  assert(!is_a_forward_pointer((stella_object *)new_location));
  return (stella_object *)new_location;
}

bool is_a_forward_pointer(stella_object *obj) {
  stella_object *new_location = check_if_is_a_forward_pointer(obj);
  return new_location != NULLPTR;
}

#endif // UTILS_H
