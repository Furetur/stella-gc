#ifndef VAR_ROOTS_H
#define VAR_ROOTS_H

#include <stella/runtime.h>

#define MAX_VAR_ROOTS (1024)
#define MAX_ROOTS_FROM_GEN0_TO_GEN1 (1024)

extern int var_roots_next_index;
extern void **var_roots[MAX_VAR_ROOTS];

extern int roots_from_gen0_to_gen1_next_index;
extern void **roots_from_gen0_to_gen1[MAX_ROOTS_FROM_GEN0_TO_GEN1];

extern int roots_from_gen1_to_gen0_next_index;
extern void **roots_from_gen1_to_gen0[MAX_ROOTS_FROM_GEN0_TO_GEN1];

// Local roots
void push_var_root(void **root);
void pop_var_root(void **root);

// Roots from Gen0 to Gen1
void scan_gen0_for_roots_to_gen1(void);

// Roots from Gen1 to Gen0
void scan_gen1_for_roots_to_gen0(void);

#endif // VAR_ROOTS_H
