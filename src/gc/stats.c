#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc/stats.h"

#include "gc/gen1.h"
#include "gc/parameters.h"
#include "gc/roots.h"

size_t total_allocated_bytes = 0;
uint64_t total_allocated_objects = 0;
size_t max_allocated_memory = 0;
uint64_t max_n_gc_roots = 0;
uint64_t n_collects = 0;

void stats_record_push_root(void) {
  uint64_t next_n_roots = var_roots_next_index + 1;
  if (next_n_roots > max_n_gc_roots) {
    max_n_gc_roots = next_n_roots;
  }
}

void add_allocation_to_total(size_t size_in_bytes) {
  total_allocated_objects += 1;
  total_allocated_bytes += size_in_bytes;
}

void record_current_usage(uint8_t *alloc_ptr) {
  uint64_t current_allocated_memory = alloc_ptr - gen1_fromspace;
  if (current_allocated_memory > max_allocated_memory) {
    max_allocated_memory = current_allocated_memory;
  }
}

void stats_record_allocation(size_t size_in_bytes) {
  add_allocation_to_total(size_in_bytes);
  record_current_usage(gen1_alloc_ptr + size_in_bytes);
}

void stats_record_collect(void) { n_collects += 1; }

void print_stats(void) {
  printf("MAX_ALLOC_SIZE:                  %zu bytes\n", GEN1_SPACE_SIZE);
  printf("Total memory allocation:         %'zu bytes (%llu objects)\n",
         total_allocated_bytes, total_allocated_objects);
  printf("Maximum residency:               %'zu bytes\n", max_allocated_memory);
  printf("Total number of GC cycles:       %'llu times\n", n_collects);
  printf("Maximum number of roots:         %'llu\n", max_n_gc_roots);
}
