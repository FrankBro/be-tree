#include "utils.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

int64_t random_in_range(int64_t min, int64_t max)
{
    return rand() % (max + 1 - min) + min;
}

bool feq(double a, double b) 
{
    return fabs(a - b) < __DBL_EPSILON__;
}

bool fne(double a, double b)
{
    return !feq(a, b);
}
