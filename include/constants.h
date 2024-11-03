#include <limits.h>

#include <runtime.h>


#define KILOBYTE 1024
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define NULLPTR ((void *)0)

#ifndef MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE ((size_t)200 * MEGABYTE)
#endif

#define MAX_THEORETICAL_SPACE_SIZE (256 * MEGABYTE)
#define SPACE_SIZE (MAX_ALLOC_SIZE)

#ifndef MAX_GC_ROOTS
#define MAX_GC_ROOTS 1024
#endif

#define TAG_MOVED (INT_MAX & TAG_MASK)
