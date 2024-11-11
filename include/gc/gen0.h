#ifndef GEN0_H
#define GEN0_H

#include <stdbool.h>
#include <stdlib.h>

#define MAX_ROOTS_FROM_GEN0_TO_GEN1 (1000)

extern bool gen0_gc_initialized;

extern uint8_t *gen0_space;

extern uint8_t *gen0_alloc_ptr;
extern uint8_t *gen1_next_ptr;
extern uint8_t *gen1_scan_ptr;

extern int next_index_roots_from_gen0_to_gen1;
extern void **roots_from_gen0_to_gen1[MAX_ROOTS_FROM_GEN0_TO_GEN1];

void gen0_initialize(void);

void *gen0_alloc(size_t size_in_bytes);

#endif // GEN0_H
