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
#include <colors.h>
#include <gfx/gfxlib.h>

static const char newline = '\n';

/**
 * Used to remove old messages and keep newest.
 * Scrolls down the terminal by moving all lines 1 step up.
 * @return void
 */
static void __terminal_scroll(struct terminal* term)
{	
	int x = 0, y = 0;
	gfx_draw_rectangle(0, 0, gfx_get_window_width(), gfx_get_window_height(), VESA8_COLOR_BLACK);
	for (int i = 0; i < term->head; i++)
	{
		if(term->textbuffer[i] == '\n'){
			x = 0;
			y++;
			continue;
		}

		gfx_draw_char(x*8, y*8, term->textbuffer[i], VESA8_COLOR_LIGHT_GREEN);
		x++;
	}
}

/**
 * Puts a given character to the terminal.
 * @param char c character to put on screen.
 * @return void
 */
void terminal_putchar(char c, struct terminal* term)
{

	if(term->head < 0 || term->head > TERMINAL_BUFFER_SIZE)
		return;

	unsigned char uc = c;	
	term->textbuffer[term->head] = c;
	term->head++;

	if(c == newline)
	{
		__terminal_scroll(term);
	}
}
 
/**
 * Writes the given string to the terminal with terminal_putchar
 * @param char* data to print to screen
 * @param int size of data
 * @return void
 */
void terminal_write(const char* data, int size, struct terminal* term)
{
	for (int i = 0; i < size; i++)
		terminal_putchar(data[i], term);
}

/**
 * Writes the given string to the terminal.
 * @param char* data to print to screen
 * @see terminal_write
 * @return void
 */
void twrite(const char* data, struct terminal* term)
{
	terminal_write(data, strlen(data), term);
}

void twriteln(const char* data, struct terminal* term)
{
	twrite(data, term);
	terminal_putchar('\n', term);
}

#define MAX_FMT_STR_SIZE 50

int32_t twritef(struct terminal* term, char* fmt, ...)
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
						twrite(str, term);
						x_offset += strlen(str);
						break;
					case 'i': ;
						num = va_arg(args, int);
						unsigned char bytes[4];
						bytes[0] = (num >> 24) & 0xFF;
						bytes[1] = (num >> 16) & 0xFF;
						bytes[2] = (num >> 8) & 0xFF;
						bytes[3] = num & 0xFF;
						twritef(term, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
						break;
					case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						twrite(str, term);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                terminal_putchar(' ', term);
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						twrite(str, term);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						twrite(str_arg, term);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						terminal_putchar(char_arg, term);
						x_offset++;
						break;
					
					default:
						terminal_putchar(*fmt, term);
						x_offset++;
						break;
				}
				fmt++;
				break;
			default:  
				terminal_putchar(*fmt, term);
				x_offset++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}
