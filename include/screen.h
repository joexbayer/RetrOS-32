#ifndef SCREEN_H
#define SCREEN_H

#include <libc.h>
#include <colors.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

/* MOVE TO vga.[c|h] */
extern uint16_t* const VGA_MEMORY;
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

void scrcolor_set(enum vga_color fg, enum vga_color bg);
void scrput(int x, int y, unsigned char c, uint8_t color);
void scrwrite(int x, int y, char* str, uint8_t color);
void scr_clear();
void scr_scroll();
int scrprintf(int32_t x, int32_t y, char* fmt, ...);
int init_vga();
uint16_t scrget(int x, int y);
uint16_t* scr_buffer();

void scr_keyboard_add(unsigned char c);
unsigned char scr_keyboard_get();

#endif