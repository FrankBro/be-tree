#include "utils.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

/*
char* strdup(const char* str)
{
    int n = strlen(str) + 1;
    char* dup = malloc(n);
    if(dup) {
        strcpy(dup, str);
    }
    return dup;
}
*/

int asprintf(char** buf, const char* format, ...)
{
    int ret;
    va_list va;

    va_start(va, format);
    ret = vasprintf(buf, format, va);
    va_end(va);
    return ret;
}

int vasprintf(char** buf, const char* format, va_list va)
{
    int len, ret;
    va_list tmp_va;
    char dummy;

    va_copy(tmp_va, va);
    len = vsnprintf(&dummy, 0, format, tmp_va);
    va_end(tmp_va);
    if(len < 0) {
        *buf = NULL;
        return len;
    }

    len += 1;
    *buf = malloc(len);
    if(*buf == NULL) {
        return -1;
    }

    ret = vsnprintf(*buf, len, format, va);
    if(ret < 0) {
        free(*buf);
        *buf = NULL;
    }

    return ret;
}

