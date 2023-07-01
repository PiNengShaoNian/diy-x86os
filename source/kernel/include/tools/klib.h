#ifndef KLIB_H
#define KLIB_H

#include <stdarg.h>
#include "comm/types.h"

static inline uint32_t down2(uint32_t size, uint32_t bound)
{
    return size & ~(bound - 1);
}

static inline uint32_t up2(uint32_t size, uint32_t bound)
{
    return (size + bound - 1) & ~(bound - 1);
}

void kernel_strcpy(char *dest, const char *src);
void kernel_strncpy(char *dest, const char *src, int size);
int kernel_strncmp(const char *s1, const char *s2, int size);
int kernel_strlen(const char *str);

void kernel_memcpy(void *dest, void *src, int size);
void kernel_memset(void *dest, uint8_t v, int size);
int kernel_memcmp(void *d1, void *d2, int size);

void kernel_sprintf(char *buf, const char *fmt, ...);
void kernel_vsprintf(char *buf, const char *fmt, va_list args);

#ifdef RELEASE
#define ASSERT(expr) ((void)0)
#else
#define ASSERT(expr) \
    if (!(expr))     \
        panic(__FILE__, __LINE__, __func__, #expr);

void panic(const char *file, int line, const char *func, const char *expr);
#endif

int strings_count(char **start);
char *get_file_name(const char *name);

#endif