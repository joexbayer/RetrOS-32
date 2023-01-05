/**
 * @file terminal.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles terminal input and currently.. UI drawing.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <terminal.h>
#include <stdarg.h>
#include <screen.h>
#include <windowmanager.h>
#include <gfx/gfxlib.h>
/*
	Main code for terminal output mainportly used for debuggin and displaying information.
	Terminal code from:
	https://wiki.osdev.org/Meaty_Skeleton
*/

static const char newline = '\n';

 /*
	TERMINAL
*/
static int terminal_row;
static int terminal_column;


static unsigned char text_buffer[2000];
static int text_head = 0;
static int text_tail = 0;

/**
 * Defines the terminal area and clears screen.
 * @return void
 */
void init_terminal(void)
{
	terminal_row = get_window_height();
	terminal_column = 0;

	/* Clears screen */
	scr_clear();

	screen_set_cursor(0, 0); 
}

/**
 * Used to remove old messages and keep newest.
 * Scrolls down the terminal by moving all lines 1 step up.
 * @return void
 */
static void __terminal_scroll()
{	
	if(text_head < 1)
		return;

	int x = 0, y = 0;
	gfx_draw_rectangle(0, 0, 300, 300, VESA8_COLOR_BLACK);
	for (int i = 0; i < text_head; i++)
	{
		if(text_buffer[i] == '\n'){
			x = 0;
			y++;
			continue;
		}

		gfx_draw_char(x*8, y*8, text_buffer[i], VESA8_COLOR_LIGHT_GREEN);
		x++;
	}

	dbgprintf("Drawing! %d\n", text_head);
	
	
}

/**
 * Puts a given character to the terminal.
 * @param char c character to put on screen.
 * @return void
 */
void terminal_putchar(char c)
{
	struct terminal_state* state = get_terminal_state();
	unsigned char uc = c;
	
	text_buffer[text_head] = c;
	text_head++;

	if(c == newline)
	{
		state->column = 1;
		__terminal_scroll();
		return;
	}

	state->column++;
}
 
/**
 * Writes the given string to the terminal with terminal_putchar
 * @param char* data to print to screen
 * @param int size of data
 * @return void
 */
void terminal_write(const char* data, int size)
{
	for (int i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

/**
 * Writes the given string to the terminal.
 * @param char* data to print to screen
 * @see terminal_write
 * @return void
 */
void twrite(const char* data)
{
	terminal_write(data, strlen(data));
}

void twriteln(const char* data)
{
	twrite(data);
	terminal_putchar('\n');
}

#define MAX_FMT_STR_SIZE 50


void terminal_fill()
{
	for (int i = 0; i < 300/8; i++)
	{
		gfx_draw_rectangle(0, i*8, 300, 8, i);
	}
}

int32_t twritef(char* fmt, ...)
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
					case 'd': ;
						num = va_arg(args, int);
						itoa(num, str);
						twrite(str);
						x_offset += strlen(str);
						break;
					case 'i': ;
						num = va_arg(args, int);
						unsigned char bytes[4];
						bytes[0] = (num >> 24) & 0xFF;
						bytes[1] = (num >> 16) & 0xFF;
						bytes[2] = (num >> 8) & 0xFF;
						bytes[3] = num & 0xFF;
						twritef("%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
						break;
					case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						twrite(str);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                terminal_putchar(' ');
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						twrite(str);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						twrite(str_arg);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						terminal_putchar(char_arg);
						x_offset++;
						break;
					
					default:
						terminal_putchar(*fmt);
						x_offset++;
						break;
				}
				fmt++;
				break;
			default:  
				terminal_putchar(*fmt);
				x_offset++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}
