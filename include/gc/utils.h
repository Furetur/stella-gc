#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include <stella/runtime.h>

// Size of stella object
size_t gc_size_of_object(stella_object *obj);

// ------------------------------------
// --- Object location checkers

bool points_to_some_space(uint8_t *space, uint8_t *ptr, size_t space_size);

bool points_to_fromspace(uint8_t *ptr);

bool points_to_tospace(uint8_t *ptr);

bool is_managed_by_gc(stella_object *obj);

bool is_enough_space_left(uint8_t *space, uint8_t *location_ptr,
                          size_t size_in_bytes);

// ------------------------------------
// --- Copy Objects

void assert_objects_equal(stella_object *obj1, stella_object *obj2);

size_t copy_object(stella_object *obj, void *dest);

#endif // UTILS_H
