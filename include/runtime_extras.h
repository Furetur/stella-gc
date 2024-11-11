#ifndef RUNTIME_EXTRAS_H
#define RUNTIME_EXTRAS_H

#include <stdbool.h>
#include <stdint.h>

#include <stella/runtime.h>

uint8_t get_tag(stella_object *obj);

void set_tag(stella_object *obj, uint8_t tag);

uint8_t get_fields_count(stella_object *obj);

void print_stella_tag(stella_object *obj);

void print_stella_object_fields(stella_object *obj);

void print_stella_object_raw(stella_object *obj);

#endif // RUNTIME_EXTRAS_H
