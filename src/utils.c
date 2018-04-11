#include "utils.h"

#include <stdint.h>
#include <stdlib.h>

uint64_t random_in_range(uint64_t min, uint64_t max)
{
    return rand() % (max + 1 - min) + min;
}
