#ifndef TERMINAL_H
#define TERMINAL_H

#include <util.h>

void init_terminal(void);
void terminal_setcolor(uint8_t color);
void twrite(const char* data);
void terminal_write(const char* data, size_t size);
void draw_mem_usage(int used);
void __terminal_putchar(char c);

void init_shell(void);
void shell_put(char c);
void shell_clear();
#endif