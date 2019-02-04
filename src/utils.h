#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"

int64_t d64min(int64_t a, int64_t b);
int64_t d64max(int64_t a, int64_t b);
uint64_t u64max(uint64_t a, uint64_t b);
size_t smin(size_t a, size_t b);
size_t smax(size_t a, size_t b);

bool feq(double a, double b);
bool fne(double a, double b);

int icmpfunc(const void *a, const void *b);
int scmpfunc(const void *a, const void *b);
int iecmpfunc(const void *a, const void *b);

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

