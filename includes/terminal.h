#ifndef TERMINAL_H
#define TERMINAL_H

#include "util.h"

void scrwrite(int x, int y, char* str);
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void twrite(const char* data);
void terminal_write(const char* data, size_t size);
void draw_mem_usage(int used);

#endif