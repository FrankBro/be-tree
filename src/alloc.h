#pragma once

#define bmalloc malloc
#define bcalloc(x) calloc(x, 1)
#define brealloc realloc
#define bfree free

#include <stdarg.h>

char* bstrdup(const char *s1);
int bvasprintf(char **buf, const char *format, va_list va);
int basprintf(char **buf, const char *format, ...);

