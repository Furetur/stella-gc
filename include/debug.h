#ifndef DEBUG_H
#define DEBUG_H

#include <runtime.h>
#include "gc_state.h"
#include "utils.h"


#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINTF(fmt, ...) printf("[debug gc] " fmt, __VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#endif


void debug_print_object(stella_object *obj) {
  printf("[debug gc]\t\tStella object at %p: ", (void *)obj);
  stella_object *new_location = check_if_is_a_forward_pointer(obj);
  if (new_location != NULLPTR) {
    printf("FORWARD POINTER TO %p", (void *)new_location);
  } else {
    size_t size = size_of_object(obj);
    printf("(size %#zx) ", size);
    print_stella_object(obj);
    printf("\n");
  }
}

#ifdef STELLA_GC_DEBUG_MODE
#define DEBUG_PRINT_OBJECT(obj) debug_print_object(obj)
#else
#define DEBUG_PRINT_OBJECT(obj) ((void)0)
#endif

#endif // DEBUG_H
