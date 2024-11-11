#include <assert.h>

#include "gc/roots.h"

#include "gc/debug.h"
#include "gc/stats.h"

int var_roots_next_index = 0;
void **var_roots[MAX_VAR_ROOTS];

int roots_from_gen0_to_gen1_next_index = 0;
void **roots_from_gen0_to_gen1[MAX_ROOTS_FROM_GEN0_TO_GEN1];

int roots_from_gen1_to_gen0_next_index = 0;
void **roots_from_gen1_to_gen0[MAX_ROOTS_FROM_GEN0_TO_GEN1];

void push_var_root(void **ptr) {
  GC_DEBUG_PRINTF("push_var_root: Pushed root %p\n", (void *)ptr);
  if (var_roots_next_index >= MAX_VAR_ROOTS) {
    printf("Out of space for roots: could not push root %p\n", (void *)ptr);
    exit(1);
  }
  stats_record_push_root();
  var_roots[var_roots_next_index++] = ptr;
}

void pop_var_root(void) {
  GC_DEBUG_PRINTF("pop_var_root: Popped root %p\n", (void *)ptr);
  assert(var_roots_next_index > 0);
  var_roots_next_index--;
}

// Roots from Gen0 to Gen1
void scan_gen0_for_roots_to_gen1(void) {}

// Roots from Gen1 to Gen0
void scan_gen1_for_roots_to_gen0(void) {}
