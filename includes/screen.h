#ifndef SCREEN_H
#define SCREEN_H

#include <util.h>

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

extern uint16_t* const VGA_MEMORY;

void scrcolor_set(enum vga_color fg, enum vga_color bg);
void scrput(size_t x, size_t y, unsigned char c, uint8_t color);
void scrwrite(int x, int y, uint8_t* str, uint8_t color);
void screen_set_cursor(int x, int y);
void scr_clear();
void scr_scroll(size_t width, size_t height);
int32_t scrprintf(int32_t x, int32_t y, uint8_t* fmt, ...);

#endif