#ifndef GC_STATE_H
#define GC_STATE_H


#include <stdint.h>

#include "constants.h"

extern uint8_t *from_space;
extern uint8_t *to_space;

extern uint8_t *allocation_ptr;
extern uint8_t *copy_ptr;

#endif // GC_STATE_H
