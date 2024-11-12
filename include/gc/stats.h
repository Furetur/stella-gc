#ifndef STATS_H
#define STATS_H

#include <stdlib.h>

void stats_record_push_root(void);

void stats_record_allocation(size_t size_in_bytes);

void stats_record_collect(int gen_n);

void stats_record_max_residency(void);

void print_stats(void);

#endif // STATS_H
