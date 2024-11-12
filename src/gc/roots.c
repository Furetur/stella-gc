#include <assert.h>
#include <stdint.h>

#include <stella/runtime.h>

#include "gc/forward_pointers.h"
#include "gc/gen0.h"
#include "gc/gen1.h"
#include "gc/parameters.h"
#include "gc/roots.h"

#include "gc/debug.h"
#include "gc/stats.h"
#include "gc/utils.h"
#include "runtime_extras.h"

int var_roots_next_index = 0;
void **var_roots[MAX_VAR_ROOTS];

int roots_from_gen0_to_gen1_next_index = 0;
void **roots_from_gen0_to_gen1[MAX_ROOTS_FROM_GEN0_TO_GEN1];

int roots_from_gen1_to_gen0_next_index = 0;
void **roots_from_gen1_to_gen0[MAX_ROOTS_FROM_GEN0_TO_GEN1];

void push_var_root(void **ptr) {
  GC_DEBUG_PRINTF("push_var_root(): Pushed root %p\n", (void *)ptr);
  if (var_roots_next_index >= MAX_VAR_ROOTS) {
    printf("Out of space for roots: could not push root %p\n", (void *)ptr);
    exit(1);
  }
  stats_record_push_root();
  var_roots[var_roots_next_index++] = ptr;
}

void pop_var_root(__attribute__((unused)) void **ptr) {
  GC_DEBUG_PRINTF("pop_var_root(): Popped root %p\n", (void *)ptr);
  assert(var_roots_next_index > 0);
  var_roots_next_index--;
}

void scan_space_for_roots(void **roots[], int *next_root_index,
                          uint8_t *space_start, uint8_t *space_end,
                          bool (*check_belongs_to_target_space)(uint8_t *)) {
  *next_root_index = 0;
  uint8_t *cur_ptr = space_start;
  while (cur_ptr < space_end) {
    stella_object *cur_obj = (stella_object *)cur_ptr;
    cur_ptr += gc_size_of_object(cur_obj);
    if (is_forward_ptr(cur_obj)) {
      void **root = &(cur_obj->object_fields[0]);
      if (check_belongs_to_target_space((uint8_t *)*root)) {
        roots[*next_root_index] = root;
        *next_root_index = *next_root_index + 1;
      }
    }
    for (int i = 0; i < get_fields_count(cur_obj); i++) {
      void **root = &(cur_obj->object_fields[i]);
      if (check_belongs_to_target_space((uint8_t *)*root)) {
        roots[*next_root_index] = root;
        *next_root_index = *next_root_index + 1;
      }
    }
  }
  assert(cur_ptr == space_end);
}

// Roots from Gen0 to Gen1
void scan_gen0_for_roots_to_gen1(void) {
  scan_space_for_roots(roots_from_gen0_to_gen1,
                       &roots_from_gen0_to_gen1_next_index, gen0_space,
                       gen0_alloc_ptr, points_to_fromspace);
  GC_DEBUG_PRINTF("scan_gen0_for_roots_to_gen1(): Scanned Gen0 "
                  "for roots and "
                  "found %d roots to Gen1\n",
                  roots_from_gen0_to_gen1_next_index);
}

// Roots from Gen1 to Gen0
void scan_gen1_for_roots_to_gen0(void) {
  scan_space_for_roots(roots_from_gen1_to_gen0,
                       &roots_from_gen1_to_gen0_next_index, gen1_fromspace,
                       gen1_alloc_ptr, points_to_gen0_space);
  GC_DEBUG_PRINTF("scan_gen1_for_roots_to_gen0(): Scanned Gen1 "
                  "for roots and "
                  "found %d roots to Gen0\n",
                  roots_from_gen1_to_gen0_next_index);
}
