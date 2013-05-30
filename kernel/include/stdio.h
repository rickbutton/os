#ifndef STDIO_H
#define STDIO_H

#include "types.h"
#include <stdarg.h>

int ksnprintf(char *str, size_t size, const char *format, ...);
int ksprintf(char *str, const char *format, ...);
int kprintf(const char *format, ...);

int kvsnprintf(char *str, size_t size, const char *format, va_list ap);

int kprint_bitmask(const char *mask, uint64_t value);
int ksnprint_bitmask(char *str, size_t size, const char *mask, uint64_t value);


#endif
