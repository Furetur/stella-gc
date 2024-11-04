#ifndef FORWARD_POINTER_H
#define FORWARD_POINTER_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "gc_state.h"
#include "utils.h"
#include <runtime.h>

const uint64_t TAG_WIDTH = 4;
const uint64_t INT_WIDTH = 32;
const uint32_t FORWARD_PTR_MASK =
    (((uint64_t)1) << INT_WIDTH) - (1 << TAG_WIDTH);
const uint32_t MAX_FORWARD_PTR_OFFSET = FORWARD_PTR_MASK >> TAG_WIDTH;

// Read forward pointer offset from the object
uint32_t read_forward_pointer_offset(stella_object *obj) {
  return ((uint32_t)obj->object_header) >> TAG_WIDTH;
}

// Read offset into the object
void write_forward_pointer_offset(stella_object *obj, uint32_t offset) {
  assert(offset <= MAX_FORWARD_PTR_OFFSET);
  int tag = obj->object_header & TAG_MASK;
  uint32_t shifted_offset = offset << TAG_WIDTH;
  assert((shifted_offset & FORWARD_PTR_MASK) == shifted_offset);
  uint32_t new_header = tag | shifted_offset;
  obj->object_header = new_header;
  // Verify
  uint32_t read_offset = read_forward_pointer_offset(obj);
  assert(read_offset == offset);
}

bool is_a_forward_pointer(stella_object *obj);

stella_object *check_if_is_a_forward_pointer(stella_object *obj) {
  int header = obj->object_header;
  int tag = STELLA_OBJECT_HEADER_TAG(header);
  if (tag != TAG_MOVED) {
    return NULLPTR;
  }
  uint32_t new_location_offset = read_forward_pointer_offset(obj); 
  uint8_t *new_location = (to_space + new_location_offset);
  assert(is_in_to_space(new_location));
  assert(!is_a_forward_pointer((stella_object *)new_location));
  return (stella_object *)new_location;
}

bool is_a_forward_pointer(stella_object *obj) {
  stella_object *new_location = check_if_is_a_forward_pointer(obj);
  return new_location != NULLPTR;
}

void set_forward_pointer(stella_object *obj, stella_object *new_obj_location) {
  assert(is_in_from_space((uint8_t *)obj));
  assert(is_in_to_space((uint8_t *)new_obj_location));
  uint64_t new_location_offset = ((uint8_t *)new_obj_location) - to_space;
  assert(new_location_offset <= UINT32_MAX);
  STELLA_OBJECT_INIT_TAG(obj, TAG_MOVED);
  write_forward_pointer_offset(obj, (uint32_t)new_location_offset);
  // Check written value
  stella_object *new_read_location = check_if_is_a_forward_pointer(obj);
  assert(new_read_location == new_obj_location);
}

#endif
