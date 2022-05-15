#ifndef TERMINAL_H
#define TERMINAL_H

#include "util.h"

void terminal_write_position(int x, int y, char* str);
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_writestring(const char* data);
void terminal_write(const char* data, size_t size);

#endif