#ifndef __IO_H
#define __IO_H

#include <stdint.h>

uint8_t inportb(uint16_t p);
uint16_t inportw(uint16_t p);
uint32_t inportl(uint16_t portid);

void outportb(uint16_t portid, uint8_t value);
void outportw(uint16_t portid, uint16_t value);
void outportl(uint16_t portid, uint32_t value);

#endif /* __IO_H */
