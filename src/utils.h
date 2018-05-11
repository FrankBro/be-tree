#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define min(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

int64_t random_in_range(int64_t min, int64_t max);
bool random_bool();

bool feq(double a, double b);
bool fne(double a, double b);
void switch_default_error(const char* str);
// char* strdup(const char* str);
int asprintf(char** buf, const char* format, ...);
int vasprintf(char** buf, const char* format, va_list va);

void betree_assert(bool test, const char* message);
