#include "utils.h"

unsigned int random_in_range(unsigned int min, unsigned int max)
{
    return rand() % (max + 1 - min) + min;
}
