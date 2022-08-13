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
/*
	Main code for terminal output mainportly used for debuggin and displaying information.
	Terminal code from:
	https://wiki.osdev.org/Meaty_Skeleton
*/

static const char newline = '\n';

#define TERMINAL_START 1
#define TERMINAL_WIDTH SCREEN_WIDTH
#define PROCESS_WIDTH (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)

 /*
	TERMINAL
*/
static int terminal_row;
static int terminal_column;
static uint8_t terminal_color;


/**
 * Clears the terminal window.
 * @return void
 */
void terminal_clear()
{	
	/* Clears the terminal window */
	for (int y = 1; y < get_window_height()-1; y++)
		for (int x = 1; x < get_window_width()-1; x++)
			scrput(x, y, ' ', terminal_color);
}

/**
 * Defines the terminal area and clears screen.
 * @return void
 */
void init_terminal(void)
{
	terminal_row = get_window_height();
	terminal_column = 0;
	terminal_color = VGA_COLOR_LIGHT_GREY;

	/* Clears screen */
	scr_clear();

	//for (int i = window.anchor+1; i < window.width-1; i++)
	//	scrput(i, 0, ' ', VGA_COLOR_BLACK | VGA_COLOR_LIGHT_GREY << 4);

	terminal_setcolor(VGA_COLOR_WHITE);
	screen_set_cursor(0, 0); 
}

/**
 * Used to remove old messages and keep newest.
 * Scrolls down the terminal by moving all lines 1 step up.
 * @return void
 */
static void __terminal_scroll()
{	
	scr_scroll(1, 1, get_window_width(), get_window_height());
}

 
/**
 * Defines color to use for printing.
 * @param uint8_t color of the text written to terminal.
 * @return void
 */
void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
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

	if(c == newline)
	{
		state->column = 1;
		__terminal_scroll();
		return;
	}
	
	if (state->column+1 == get_window_width()-1)
	{
		scrput(state->column, get_window_height()-1, '-', terminal_color);
		state->column = 1;
		__terminal_scroll();
	}
	scrput(state->column, get_window_height()-1, uc, terminal_color);
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
	//__terminal_putchar('<');
	//__terminal_putchar(' ');
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