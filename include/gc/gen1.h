#ifndef GEN1_H
#define GEN1_H

#include <stdbool.h>
#include <stdlib.h>

extern bool gen1_gc_initialized;

extern uint8_t *gen1_fromspace;
extern uint8_t *gen1_tospace;

extern uint8_t *gen1_alloc_ptr;
extern uint8_t *gen1_next_ptr;
extern uint8_t *gen1_scan_ptr;

void gen1_initialize(void);

void *gen1_alloc(size_t size_in_bytes);

#endif // GEN1_H
