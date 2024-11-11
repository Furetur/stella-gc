#include <assert.h>
#include <stdint.h>

#include <stella/runtime.h>

#include "gc/forward_pointers.h"

#include "constants.h"
#include "runtime_extras.h"

stella_object *as_forward_ptr(stella_object *obj) {
  if (get_tag(obj) != TAG_FORWARD_PTR) {
    return NULLPTR;
  }
  assert(get_fields_count(obj) >= 1);
  void *forward_ptr = obj->object_fields[0];
  return (stella_object *)forward_ptr;
}

void set_forward_ptr(stella_object *obj, stella_object *new_location) {
  assert(get_fields_count(obj) >= 1);
  set_tag(obj, TAG_FORWARD_PTR);
  obj->object_fields[0] = (void *)new_location;
  // Verify
  stella_object *read_location = as_forward_ptr(obj);
  assert(read_location == new_location);
}

bool is_forward_ptr(stella_object *obj) {
  return as_forward_ptr(obj) != NULLPTR;
}
