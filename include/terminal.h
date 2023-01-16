#ifndef TERMINAL_H
#define TERMINAL_H

#include <util.h>

#define TERMINAL_BUFFER_SIZE 2000
struct terminal {
    char textbuffer[TERMINAL_BUFFER_SIZE];
    int tail;
    int head;
};

void twrite(const char* data);
void terminal_write(const char* data, int size);
void draw_mem_usage(int used);
void terminal_putchar(char c);
int32_t twritef(char* fmt, ...);


void terminal_commit();

void terminal_attach(struct terminal* term);


void twriteln(const char* data);

#endif