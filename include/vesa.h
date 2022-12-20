#ifndef DFA8C135_4052_4480_8A44_09EA9D67997D
#define DFA8C135_4052_4480_8A44_09EA9D67997D

#include <stdint.h>

#define PIXELS_PER_CHAR 8
#define PIXELS_PER_ICON 16

void vesa_put_char(unsigned char c, int x, int y);
void vesa_put_icon(int x, int y);
void vesa_write_str(int x, int y, const char* data);
void vesa_fill(unsigned char color);

void vesa_put_pixel(int x,int y, unsigned char color);

#endif /* DFA8C135_4052_4480_8A44_09EA9D67997D */
