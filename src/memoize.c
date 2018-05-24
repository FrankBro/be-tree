#include <stdint.h>

#include "memoize.h"

void set_bit(uint64_t A[], uint64_t k)
{
    A[k / 64] |= 1 << (k % 64);
}

void clear_bit(uint64_t A[], uint64_t k)
{
    A[k / 64] &= ~(1 << (k % 64));
}

int test_bit(uint64_t A[], uint64_t k)
{
    return ((A[k / 64] & (1 << (k % 64))) != 0);
}
