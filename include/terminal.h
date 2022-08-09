#ifndef TERMINAL_H
#define TERMINAL_H

#include <util.h>

void init_terminal(void);
void terminal_setcolor(uint8_t color);
void twrite(const char* data);
void terminal_write(const char* data, int size);
void draw_mem_usage(int used);
void terminal_putchar(char c);
int32_t twritef(char* fmt, ...);


void twriteln(const char* data);

#endif