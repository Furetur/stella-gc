#ifndef DEBUG_H
#define DEBUG_H

#include "gc_state.h"
#include "utils.h"
#include <assert.h>
#include <runtime.h>
#include <stdio.h>

#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINTF(fmt, ...) printf("[debug gc] " fmt, __VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#endif

void print_fields(stella_object *obj) {
  int n_fields = STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
  printf("{");
  for (int i = 0; i < n_fields; i++) {
    printf("%p", obj->object_fields[i]);
    if (i < n_fields - 1) {
      printf(", ");
    }
  }
  printf("}");
}

void verbose_print_stella_object(stella_object *obj) {
  size_t size = size_of_object(obj);
  int n_fields = STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
  printf("(size %#zx, n_fields %d) ", size, n_fields);
  print_fields(obj);
  printf(" ");
  print_stella_object(obj);
}

void debug_print_object(stella_object *obj) {
  printf("[debug gc]\t\tStella object at %p[%c]: ", (void *)obj,
         get_object_location_kind(obj));
  if (obj == NULLPTR) {
    printf("nullptr\n");
    return;
  } else if (!is_managed_by_gc(obj)) {
    printf("UNMANAGED\n");
    return;
  }
  stella_object *new_location = check_if_is_a_forward_pointer(obj);
  if (new_location != NULLPTR) {
    printf("FORWARD POINTER TO %p[%c]: ", (void *)new_location,
           get_object_location_kind(new_location));
    assert(!is_a_forward_pointer(new_location));
    verbose_print_stella_object(new_location);
  } else {
    verbose_print_stella_object(obj);
  }
  printf("\n");
}

#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINT_OBJECT(obj) debug_print_object(obj)
#else
#define DEBUG_PRINT_OBJECT(obj) ((void)0)
#endif

#endif // DEBUG_H
