#ifndef DEBUG_H
#define DEBUG_H

#include <assert.h>
#include <stdio.h>

#include <runtime.h>

#include "constants.h"
#include "gc/forward_pointers.h"
#include "gc/state.h"
#include "runtime_extras.h"
#include "utils.h"

#ifdef STELLA_GC_DEBUG_MODE
#define GC_DEBUG_PRINTF(fmt, ...) printf("[debug gc] " fmt, __VA_ARGS__)
#else
#define GC_DEBUG_PRINTF(fmt, ...) ((void)0)
#endif

char *LOCATION_FROMSPACE = "FROM-SPACE";
char *LOCATION_TOSPACE = "TO-SPACE";
char *LOCATION_UNMANAGED_SPACE = "UNMANAGED SPACE";

char *describe_object_location(stella_object *obj) {
  if (!is_managed_by_gc(obj)) {
    return LOCATION_UNMANAGED_SPACE;
  } else if (points_to_fromspace((void *)obj)) {
    return LOCATION_FROMSPACE;
  } else {
    assert(points_to_tospace((void *)obj));
    return LOCATION_TOSPACE;
  }
}

void print_gc_object(stella_object *obj) {
  size_t size = gc_size_of_object(obj);
  printf("object at %p (%s) of size %#zx: ", (void *)obj,
         describe_object_location(obj), size);
  if (obj == NULLPTR) {
    printf("nullptr");
    return;
  }
  print_stella_object_raw(obj);
  stella_object *forward_ptr = as_forward_ptr(obj);
  if (forward_ptr != NULLPTR) {
    printf(" ---FWD---> ");
    print_gc_object(forward_ptr);
  }
}

void debug_print_object(stella_object *obj) {
  printf("[debug gc]        Note: ");
  print_gc_object(obj);
  printf("\n");
}

#ifdef STELLA_GC_DEBUG_MODE
#define GC_DEBUG_PRINT_OBJECT(obj) debug_print_object(obj)
#else
#define GC_DEBUG_PRINT_OBJECT(obj) ((void)0)
#endif

#endif // DEBUG_H
