#ifndef __PRINTF_H
#define __PRINTF_H

#include <lib/syscall.h>
#include <args.h>

#ifdef __cplusplus
extern "C"
{
#endif

void print_write(const char* data, int size);
void print(const char* data);
void println(const char* data);
int printf(const char* fmt, ...);
int sprintf(char *buffer, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* __PRINTF_H */
