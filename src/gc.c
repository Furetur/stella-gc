#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gc.h>
#include <runtime.h>

#include "constants.h"
#include "gc/gen0.h"
#include "gc/parameters.h"
#include "gc/roots.h"
#include "gc/stats.h"
#include "runtime_extras.h"

#include "gc/gen1.h"

// ------------------------------------
// --- GC State

bool gc_initialized = false;

void initialize_gc_if_needed(void) {
  if (gc_initialized) {
    return;
  }
  gen0_initialize();
  gen1_initialize();
  gc_initialized = true;
}

void *gc_alloc(size_t size_in_bytes) {
  initialize_gc_if_needed();
  return gen0_alloc(size_in_bytes);
}

void print_gc_roots(void) {
  initialize_gc_if_needed();
  printf("List of GC roots (%d elements):\n", var_roots_next_index);
  for (int i = 0; i < var_roots_next_index; i++) {
    printf("  %d. Root %p points at object %p\n", i + 1,
           (void *)(void *)var_roots[i], (void *)*var_roots[i]);
  }
}

void print_gc_alloc_stats(void) { print_stats(); }

void print_gc_state(void) {
  initialize_gc_if_needed();
  printf("from-space: %p..%p\n", (void *)gen1_fromspace,
         (void *)(gen1_fromspace + GEN1_SPACE_SIZE - 1));
  printf("to-space: %p..%p\n", (void *)gen1_tospace,
         (void *)(gen1_fromspace + GEN1_SPACE_SIZE - 1));
  printf("alloc_ptr: %p\n", (void *)gen1_alloc_ptr);
  size_t allocated_bytes = gen1_alloc_ptr - gen1_fromspace;
  size_t free_bytes = GEN1_SPACE_SIZE - allocated_bytes;
  printf("    allocated in from-space: %#zx bytes\n", allocated_bytes);
  printf("    free in from-space:      %#zx bytes\n", free_bytes);
  size_t allocated_in_tospace = gen1_next_ptr - gen1_tospace;
  size_t free_in_tospace = GEN1_SPACE_SIZE - allocated_bytes;
  printf("next_ptr: %p\n", (void *)gen1_next_ptr);
  printf("    allocated in to-space:   %#zx bytes\n", allocated_in_tospace);
  printf("    free in to-space:        %#zx bytes\n", free_in_tospace);
  printf("scan_ptr: %p\n", (void *)gen1_scan_ptr);
  printf("    left to scan:            %#zx bytes\n",
         gen1_next_ptr - gen1_scan_ptr);
  print_gc_roots();
}

void gc_read_barrier(__attribute__((unused)) void *object,
                     __attribute__((unused)) int field_index) {}

void gc_write_barrier(__attribute__((unused)) void *object,
                      __attribute__((unused)) int field_index,
                      __attribute__((unused)) void *contents) {}

void gc_push_root(void **ptr) { push_var_root(ptr); }

void gc_pop_root(void **ptr) { pop_var_root(ptr); }
