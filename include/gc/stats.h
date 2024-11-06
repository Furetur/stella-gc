#ifndef STATS_H
#define STATS_H

#include <stdlib.h>

void stats_record_push_root(void);

void stats_record_allocation(size_t size_in_bytes);

void stats_record_collect(void);

void print_stats(void);

#endif // STATS_H
