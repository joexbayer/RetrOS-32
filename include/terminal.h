#ifndef TERMINAL_H
#define TERMINAL_H

#include <util.h>

#define TERMINAL_BUFFER_SIZE 2000
struct terminal {
    char textbuffer[TERMINAL_BUFFER_SIZE];
    int tail;
    int head;
};

void twrite(const char* data, struct terminal* term);
void terminal_write(const char* data, int size, struct terminal* term);
void draw_mem_usage(int used, struct terminal* term);
void terminal_putchar(char c, struct terminal* term);
int32_t twritef(struct terminal* term, char* fmt, ...);


void twriteln(const char* data, struct terminal* term);

#endif