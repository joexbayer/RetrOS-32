#ifndef __PRINTF_H
#define __PRINTF_H

#include <lib/syscall.h>

#ifdef __cplusplus
extern "C"
{
#endif

void print_write(const char* data, int size);
void print(const char* data);
void println(const char* data);
int printf(char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __PRINTF_H */
