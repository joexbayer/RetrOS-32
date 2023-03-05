#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>
#include <stdarg.h>

#define dbgprintf(a, ...) serial_printf("%s(): " a, __func__, ##__VA_ARGS__)

void init_serial();
int32_t serial_printf(char* fmt, ...);
void serial_put(char a);
#endif /* __SERIAL_H */
