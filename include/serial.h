#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>
#include <kconfig.h>
#include <args.h>

#ifdef KDEBUG_SERIAL
#define dbgprintf(a, ...) serial_printf("%s(): " a, __func__, ##__VA_ARGS__)
#else
#define dbgprintf(a, ...)
#endif // KDEBUG_SERIAL

#ifdef KDEBUG_WARNINGS
#define warningf(a, ...) serial_printf("DEBUG: %s(): " a, __func__, ##__VA_ARGS__)
#else
#define warningf(a, ...)
#endif // KDEBUG_WARNINGS

void init_serial();
int32_t serial_printf(char* fmt, ...);
void serial_put(char a);
#endif /* __SERIAL_H */
