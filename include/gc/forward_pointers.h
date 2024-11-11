#ifndef FORWARD_POINTERS_H
#define FORWARD_POINTERS_H

#include <stdbool.h>

#include <stella/runtime.h>

#define TAG_FORWARD_PTR ((uint8_t)TAG_MASK)

stella_object *as_forward_ptr(stella_object *obj);

void set_forward_ptr(stella_object *obj, stella_object *new_location);

bool is_forward_ptr(stella_object *obj);
#endif // FORWARD_POINTERS_H
