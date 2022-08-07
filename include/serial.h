#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

void init_serial();
int32_t dbgprintf(char* fmt, ...);
#endif /* __SERIAL_H */
