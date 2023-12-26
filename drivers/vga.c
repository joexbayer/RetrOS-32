/**
 * @file vga.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles drawing to the screen, mainly in VGA_MEMORY
 * @version 0.2
 * @date 2022-06-01
 * @date 2023-12-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <screen.h>
#include <pcb.h>
#include <windowmanager.h>
#include <args.h>
#include <arch/io.h>
#include <serial.h>
#include <vbe.h>
#include <gfx/gfxlib.h>

#define MAX_FMT_STR_SIZE 50

uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;
//uint16_t* const VGA_MEMORY = (uint16_t*) 0xa0000;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
static uint8_t scrcolor = VGA_COLOR_WHITE | VGA_COLOR_BLUE << 4;
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

void screen_set_cursor(int x, int y)
{
	uint16_t pos = y * SCREEN_WIDTH + x + 1;
	outportb(0x3D4, 0x0F);
	outportb(0x3D5, (uint8_t) (pos & 0xFF));
	outportb(0x3D4, 0x0E);
	outportb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void scrcolor_set(enum vga_color fg, enum vga_color bg)
{
	scrcolor = vga_entry_color(fg, bg);
}

/**
 * Puts a given character to the specified screen location.
 * Index calculation = y * SCREEN_WIDTH + x;
 * @param int x coordinate
 * @param int y coordinate
 * @param char c character to put on screen.
 * @param uint8_t color of character
 * @return void
 */
void scrput(int x, int y, unsigned char c, uint8_t color)
{
	const int index = y * SCREEN_WIDTH + x;
	VGA_MEMORY[index] = vga_entry(c, scrcolor);
}

/**
 * Writes given string to terminal at specified position.
 * @param int x coordinate of screen.
 * @param int y coordinate of screen.
 * @param char* str string to print.
 * @param uint8_t color of string
 * @return void
 */
void scrwrite(int x, int y, char* str, uint8_t color)
{
	for (int i = 0; i < strlen(str); i++){
		scrput(x+i, y, str[i], scrcolor);
	}
}

/**
 * Clears the entire screen.
 * @return void
 */
void scr_clear()
{
    ENTER_CRITICAL();
	for (int y = 0; y < SCREEN_HEIGHT; y++){
		for (int x = 0; x < SCREEN_WIDTH; x++){
			scrput(x, y, ' ', scrcolor);
		}
	}
    LEAVE_CRITICAL();
	
}

void scr_scroll()
{
    /* Move all lines up, overwriting the oldest message. */
	for (int y = 0; y < SCREEN_HEIGHT-1; y++){
		for (int x = 0; x < SCREEN_WIDTH; x++){
			const int index = y * SCREEN_WIDTH + x;
			const int index_b = (y+1) * SCREEN_WIDTH + x;

			VGA_MEMORY[index] = VGA_MEMORY[index_b];
		}
	}
}

/**
 * Writes the given string with formats to screen on give location.
 * @param int x coordinate
 * @param int y coordinate
 * @param char* format string
 * @param ... variable parameters
 * @return number of bytes written
 */
int32_t scrprintf(int32_t x, int32_t y, char* fmt, ...)
{
	va_list args;

	int x_offset = 0;
	int written = 0;
	char str[MAX_FMT_STR_SIZE];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt)
		{
			case '%':
				memset(str, 0, MAX_FMT_STR_SIZE);
				switch (*(fmt+1))
				{
					case 'd':
					case 'i': ;
						num = va_arg(args, int);
						itoa(num, str);
						scrwrite(x+x_offset, y, str, scrcolor);
						x_offset += strlen(str);
						break;
                    case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						scrwrite(x+x_offset, y, str, scrcolor);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                scrput(x+x_offset, y, ' ', scrcolor);
                                x_offset++;
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						scrwrite(x+x_offset, y, str, scrcolor);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						scrwrite(x+x_offset, y, str_arg, scrcolor);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						scrput(x+x_offset, y, char_arg, scrcolor);
						x_offset++;
						break;
					default:
                        break;
				}
				fmt++;
				break;
			case '\n':
				y++;
				written += x_offset;
				x_offset = 0;
				break;
			default:  
				scrput(x+x_offset, y, *fmt, scrcolor);
				x_offset++;
                written++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}