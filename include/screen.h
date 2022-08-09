#ifndef SCREEN_H
#define SCREEN_H

#include <util.h>
#include <colors.h>



#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 50

extern uint16_t* const VGA_MEMORY;

void scrcolor_set(enum vga_color fg, enum vga_color bg);
void scrput(int x, int y, unsigned char c, uint8_t color);
void scrwrite(int x, int y, char* str, uint8_t color);
void screen_set_cursor(int x, int y);
void scr_clear();
void scr_scroll(int from, int to, int width, int height);
int32_t scrprintf(int32_t x, int32_t y, char* fmt, ...);

#endif