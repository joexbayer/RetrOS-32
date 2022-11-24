#ifndef __PRINTF_H
#define __PRINTF_H

void print_write(const char* data, int size);
void print(const char* data);
void println(const char* data);
int printf(char* fmt, ...);

#endif /* __PRINTF_H */
