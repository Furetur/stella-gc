#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "constants.h"

#ifndef MAX_ALLOC_SIZE
#define MAX_ALLOC_SIZE ((size_t)GIGABYTE)
#endif

#define GEN0_SPACE_SIZE (MAX_ALLOC_SIZE / 3)
#define GEN1_SPACE_SIZE (GEN0_SPACE_SIZE * 2)

#endif // PARAMETERS_H
