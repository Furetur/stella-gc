#ifndef GC_STATE_H
#define GC_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "constants.h"

// ------------------------------------
// --- Constants

#ifndef MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE ((size_t)GIGABYTE)
#endif

#define SPACE_SIZE (MAX_ALLOC_SIZE)

#ifndef MAX_GC_ROOTS
#define MAX_GC_ROOTS 1024
#endif

// ------------------------------------
// --- GC State

extern bool gc_initialized;

extern uint8_t *fromspace;
extern uint8_t *tospace;

extern uint8_t *alloc_ptr;
extern uint8_t *next_ptr;
extern uint8_t *scan_ptr;

extern int gc_roots_next_index;
extern void **gc_roots[MAX_GC_ROOTS];

#endif // GC_STATE_H
