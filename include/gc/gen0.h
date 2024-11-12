#ifndef GEN0_H
#define GEN0_H

#include <stdbool.h>
#include <stdlib.h>

extern bool gen0_gc_initialized;

extern uint8_t *gen0_space;

extern uint8_t *gen0_alloc_ptr;
extern uint8_t *gen0_scan_ptr;

void gen0_initialize(void);

void *gen0_alloc(size_t size_in_bytes);

#endif // GEN0_H
