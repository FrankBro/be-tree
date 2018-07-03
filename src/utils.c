#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betree.h"
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

int icmpfunc(const void *a, const void *b) 
{
  const int64_t x = *(int64_t*)a;
  const int64_t y = *(int64_t*)b;
  int64_t comp =  x - y;
  if (comp < 0)
    return -1;
  if (comp > 0)
    return 1;
  return comp;
}

int scmpfunc(const void *a, const void *b) 
{
  const struct string_value* x = (struct string_value*)a;
  const struct string_value* y = (struct string_value*)b;
  if (x->str < y->str)
    return -1;
  if (x->str > y->str)
    return 1;
  return 0;
}

