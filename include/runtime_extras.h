#ifndef RUNTIME_EXTRAS_H
#define RUNTIME_EXTRAS_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdint.h>
#include <stdio.h>
#include <stella/runtime.h>

uint8_t get_tag(stella_object *obj) {
  return STELLA_OBJECT_HEADER_TAG(obj->object_header);
}

void set_tag(stella_object *obj, uint8_t tag) {
  assert((tag & TAG_MASK) == tag);
  STELLA_OBJECT_INIT_TAG(obj, tag);
  // Verify
  uint8_t read_tag = get_tag(obj);
  assert(read_tag == tag);
}

uint8_t get_fields_count(stella_object *obj) {
  return STELLA_OBJECT_HEADER_FIELD_COUNT(obj->object_header);
}

void print_stella_tag(stella_object *obj) {
  int tag = STELLA_OBJECT_HEADER_TAG(obj->object_header);
  switch (tag) {
  case TAG_ZERO:
    printf("TAG_ZERO");
    break;
  case TAG_SUCC:
    printf("TAG_SUCC");
    break;
  case TAG_FALSE:
    printf("TAG_FALSE");
    break;
  case TAG_TRUE:
    printf("TAG_TRUE");
    break;
  case TAG_FN:
    printf("TAG_FN");
    break;
  case TAG_REF:
    printf("TAG_REF");
    break;
  case TAG_UNIT:
    printf("TAG_UNIT");
    break;
  case TAG_TUPLE:
    printf("TAG_TUPLE");
    break;
  case TAG_INL:
    printf("TAG_INL");
    break;
  case TAG_INR:
    printf("TAG_INR");
    break;
  case TAG_EMPTY:
    printf("TAG_EMPTY");
    break;
  case TAG_CONS:
    printf("TAG_CONS");
    break;
  default:
    assert(false);
  };
}

void print_stella_object_fields(stella_object *obj) {
  uint8_t n_fields = get_fields_count(obj);
  printf("[");
  for (int i = 0; i < n_fields; i++) {
    printf("%p", obj->object_fields[i]);
    if (i < n_fields - 1) {
      printf(", ");
    }
  }
  printf("]");
}

void print_stella_object_raw(stella_object *obj) {
  printf("[");
  print_stella_tag(obj);
  printf(", ");
  print_stella_object_fields(obj);
  printf("]");
}

#endif // RUNTIME_EXTRAS_H
