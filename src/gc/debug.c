#include <assert.h>
#include <stdio.h>

#include <stella/runtime.h>

#include "gc/debug.h"

#include "constants.h"
#include "gc/forward_pointers.h"
#include "gc/utils.h"
#include "runtime_extras.h"

static char *LOCATION_GEN0_SPACE = "GEN0-SPACE";
static char *LOCATION_FROMSPACE = "FROM-SPACE";
static char *LOCATION_TOSPACE = "TO-SPACE";
static char *LOCATION_UNMANAGED_SPACE = "UNMANAGED SPACE";

char *describe_object_location(stella_object *obj) {
  if (!is_managed_by_gc(obj)) {
    return LOCATION_UNMANAGED_SPACE;
  } else if (points_to_gen0_space((void *)obj)) {
    return LOCATION_GEN0_SPACE;
  } else if (points_to_fromspace((void *)obj)) {
    return LOCATION_FROMSPACE;
  } else {
    assert(points_to_tospace((void *)obj));
    return LOCATION_TOSPACE;
  }
}

void print_gc_object(stella_object *obj, bool allow_forward_ptr) {
  size_t size = gc_size_of_object(obj);
  printf("object at %p (%s) of size %#zx: ", (void *)obj,
         describe_object_location(obj), size);
  if (obj == NULLPTR) {
    printf("nullptr");
    return;
  }
  if (!is_managed_by_gc(obj)) {
    printf("unknown object from unmanaged space");
    return;
  }
  stella_object *forward_ptr = as_forward_ptr(obj);
  if (forward_ptr != NULLPTR) {
    assert(allow_forward_ptr);
    printf("FWD to %p: ", (void *)forward_ptr);
    print_gc_object(forward_ptr, false);
  } else {
    print_stella_object_raw(obj);
  }
}

void gc_debug_print_object(stella_object *obj) {
  printf("[debug gc]        Note: ");
  print_gc_object(obj, true);
  printf("\n");
}
