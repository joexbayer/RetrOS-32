#ifndef TERMINAL_H
#define TERMINAL_H

#include <util.h>

void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void twrite(const char* data);
void terminal_write(const char* data, size_t size);
void draw_mem_usage(int used);
void __terminal_putchar(char c);

void shell_init(void);
void shell_put(char c);
void shell_clear();

#endif