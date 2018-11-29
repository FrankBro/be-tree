#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "alloc.h"

#ifdef NIF
#include "erl_nif.h"
void* enif_calloc(size_t size)
{
    void* ptr = enif_alloc(size);
    if(ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}
#endif

char* bstrdup(const char *s1)
{
  char *str;
  size_t size = strlen(s1) + 1;

  str = bmalloc(size);
  if (str) {
    memcpy(str, s1, size);
  }
  return str;
}

__attribute__((__format__ (__printf__, 2, 0)))
int bvasprintf(char **buf, const char *format, va_list va)
{
	int len, ret;
	va_list tmp_va;
	char dummy;

	va_copy(tmp_va, va);
	len = vsnprintf(&dummy, 0, format, tmp_va);
	va_end(tmp_va);
	if (len < 0) {
		*buf = NULL;
		return (len);
	}

	/* Account for null terminator. */
	len += 1;
	*buf = bmalloc(len);
	if (*buf == NULL)
		return (-1);

	ret = vsnprintf(*buf, len, format, va);
	if (ret < 0) {
		free(*buf);
		*buf = NULL;
	}

	return (ret);
}

__attribute__((__format__ (__printf__, 2, 0)))
int basprintf(char **buf, const char *format, ...)
{
	int ret;
	va_list va;

	va_start(va, format);
	ret = bvasprintf(buf, format, va);
	va_end(va);

	return (ret);
}

