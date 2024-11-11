#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>

#include <stella/runtime.h>

#ifdef STELLA_GC_DEBUG_MODE
#define GC_DEBUG_PRINTF(fmt, ...) printf("[debug gc] " fmt, __VA_ARGS__)
#else
#define GC_DEBUG_PRINTF(fmt, ...) ((void)0)
#endif

char *describe_object_location(stella_object *obj);

void print_gc_object(stella_object *obj, bool allow_forward_ptr);

void gc_debug_print_object(stella_object *obj);

#ifdef STELLA_GC_DEBUG_MODE
#define GC_DEBUG_PRINT_OBJECT(obj) gc_debug_print_object(obj)
#else
#define GC_DEBUG_PRINT_OBJECT(obj) ((void)0)
#endif

#endif // DEBUG_H
