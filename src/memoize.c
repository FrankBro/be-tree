#include <stdint.h>

#include "memoize.h"

void set_bit(uint64_t A[], uint64_t k)
{
    A[k / 64ULL] |= (1ULL << (k % 64ULL));
}

void clear_bit(uint64_t A[], uint64_t k)
{
    A[k / 64ULL] &= (~(1ULL << (k % 64ULL)));
}

bool test_bit(uint64_t A[], uint64_t k)
{
    return ((A[k / 64ULL] & (1ULL << (k % 64ULL))) != 0ULL);
}

