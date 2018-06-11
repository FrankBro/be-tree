#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

bool bmin(bool a, bool b)
{
    return a < b ? a : b;
}

bool bmax(bool a, bool b)
{
    return a > b ? a : b;
}

int64_t d64min(int64_t a, int64_t b)
{
    return a < b ? a : b;
}

int64_t d64max(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

uint64_t u64min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

uint64_t u64max(uint64_t a, uint64_t b)
{
    return a > b ? a : b;
}

size_t smin(size_t a, size_t b)
{
    return a < b ? a : b;
}

size_t smax(size_t a, size_t b)
{
    return a > b ? a : b;
}

int64_t random_in_range(int64_t min, int64_t max)
{
    return rand() % (max + 1 - min) + min;
}

bool random_bool()
{
    return random_in_range(false, true);
}

bool feq(double a, double b)
{
    return fabs(a - b) < __DBL_EPSILON__;
}

bool fne(double a, double b)
{
    return !feq(a, b);
}

void switch_default_error(const char* str)
{
    fprintf(stderr, "%s", str);
    abort();
}

void betree_assert(bool should_abort, enum error_e error, bool expr)
{
    (void)error;
    if(!expr) {
        if(should_abort) {
            abort();
        }
    }
}

