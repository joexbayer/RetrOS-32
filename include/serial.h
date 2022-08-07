#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

void init_serial();
int32_t dbgprintf(char* fmt, ...);
void serial_put(char a);
#endif /* __SERIAL_H */
