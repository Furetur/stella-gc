#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc/stats.h"

#include "gc/gen0.h"
#include "gc/gen1.h"
#include "gc/parameters.h"
#include "gc/roots.h"

size_t total_allocated_bytes = 0;
uint64_t total_allocated_objects = 0;
uint64_t max_n_gc_roots = 0;
uint64_t max_gen0_allocated_memory = 0;
uint64_t max_gen1_allocated_memory = 0;
uint64_t max_allocated_memory = 0;
uint64_t gen0_n_collects = 0;
uint64_t gen1_n_collects = 0;

void stats_record_push_root(void) {
  uint64_t next_n_roots = var_roots_next_index + 1;
  if (next_n_roots > max_n_gc_roots) {
    max_n_gc_roots = next_n_roots;
  }
}

static void add_allocation_to_total(size_t size_in_bytes) {
  total_allocated_objects += 1;
  total_allocated_bytes += size_in_bytes;
}

void stats_record_max_residency(void) {
  uint64_t current_gen0_allocated_memory = gen0_alloc_ptr - gen0_space;
  uint64_t current_gen1_allocated_memory = gen1_alloc_ptr - gen1_fromspace;
  if (current_gen0_allocated_memory > max_gen0_allocated_memory) {
    max_gen0_allocated_memory = current_gen0_allocated_memory;
  }
  if (current_gen1_allocated_memory > max_gen1_allocated_memory) {
    max_gen1_allocated_memory = current_gen1_allocated_memory;
  }
  uint64_t current_allocated_memory =
      current_gen0_allocated_memory + current_gen1_allocated_memory;
  if (current_allocated_memory > max_allocated_memory) {
    max_allocated_memory = current_allocated_memory;
  }
}

void stats_record_allocation(size_t size_in_bytes) {
  add_allocation_to_total(size_in_bytes);
}

void stats_record_collect(int gen_n) {
  assert((gen_n == 0) || (gen_n == 1));
  if (gen_n == 0) {
    gen0_n_collects++;
  } else {
    gen1_n_collects++;
  }
}

void print_stats(void) {
  printf("MAX_ALLOC_SIZE:                  %zu bytes\n",
         (size_t)MAX_ALLOC_SIZE);
  printf("    Gen0 space size:             %zu bytes\n", GEN0_SPACE_SIZE);
  printf("    Gen1 space size:             %zu bytes\n", GEN1_SPACE_SIZE);
  printf("Total memory allocation:         %'zu bytes (%llu objects)\n",
         total_allocated_bytes, total_allocated_objects);
  printf("Maximum residency:               %'llu bytes\n",
         max_allocated_memory);
  printf("    Gen0:                        %'llu bytes\n",
         max_gen0_allocated_memory);
  printf("    Gen1:                        %'llu bytes\n",
         max_gen1_allocated_memory);
  printf("Total number of GC cycles:       %'llu times\n",
         gen0_n_collects + gen1_n_collects);
  printf("    Gen0 cycles:                 %'llu times\n", gen0_n_collects);
  printf("    Gen1 cycles:                 %'llu times\n", gen1_n_collects);
  printf("Maximum number of roots:         %'llu\n", max_n_gc_roots);
}
