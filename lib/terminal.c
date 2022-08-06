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

enum ASCII {
	ASCII_BLOCK = 219,
	ASCII_HORIZONTAL_LINE = 205,
	ASCII_VERTICAL_LINE = 179,
	ASCII_DOWN_INTERSECT = 203
};

static const char newline = '\n';

#define TERMINAL_START 1
#define TERMINAL_WIDTH SCREEN_WIDTH
#define PROCESS_WIDTH (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)

 /*
	TERMINAL
*/
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static struct window window = {
		.anchor = 1,
		.height = SCREEN_HEIGHT-3,
		.width = SCREEN_WIDTH-3,
		.color = VGA_COLOR_LIGHT_BLUE
	};

/**
 * Draws lines on the screen separating terminal, memory and example.
 * 
 * @return void
 */
void __terminal_draw_lines()
{
	
	/*
	for (size_t x = 0; x < SCREEN_WIDTH; x++)
		scrput(x,TERMINAL_START, ASCII_HORIZONTAL_LINE, terminal_color);*/

	for (size_t x = 0; x < SCREEN_HEIGHT; x++)
		scrput(0, 0+x, ASCII_VERTICAL_LINE, VGA_COLOR_LIGHT_GREY);

	for (size_t x = 0; x < SCREEN_HEIGHT; x++)
		scrput(SCREEN_WIDTH-1, 0+x, ASCII_VERTICAL_LINE, VGA_COLOR_LIGHT_GREY);

	scrput(0, SCREEN_HEIGHT-1, 192, VGA_COLOR_LIGHT_GREY);
	scrput(SCREEN_WIDTH-1, SCREEN_HEIGHT-1, 217, VGA_COLOR_LIGHT_GREY);

	for (size_t x = 1; x < SCREEN_WIDTH-1; x++)
		scrput(x, SCREEN_HEIGHT-1, 196, VGA_COLOR_LIGHT_GREY);

}

/**
 * Clears the terminal window.
 * @return void
 */
void terminal_clear()
{	
	/* Clears the terminal window */
	for (size_t y = window.anchor+1; y < window.height-1; y++)
		for (size_t x = window.anchor+1; x < window.width-1; x++)
			scrput(x, y, ' ', terminal_color);
}

/**
 * Defines the terminal area and clears screen.
 * @return void
 */
void init_terminal(void)
{
	terminal_row = window.height;
	terminal_column = window.anchor;
	terminal_color = VGA_COLOR_LIGHT_GREY;

	/* Clears screen */
	scr_clear();


	__terminal_draw_lines();

	for (size_t i = window.anchor+1; i < window.width-1; i++)
		scrput(i, 0, ' ', VGA_COLOR_BLACK | VGA_COLOR_LIGHT_GREY << 4);

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
	scr_scroll(window.anchor+1, window.anchor+1, window.width, window.height);
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
	unsigned char uc = c;

	if(c == newline)
	{
		terminal_column = window.anchor+1;
		__terminal_scroll();
		return;
	}
	
	if (terminal_column == window.width-1)
	{
		return;
	}
	scrput(terminal_column, terminal_row, uc, terminal_color);
	terminal_column++;
}
 
/**
 * Writes the given string to the terminal with terminal_putchar
 * @param char* data to print to screen
 * @param size_t size of data
 * @return void
 */
void terminal_write(const char* data, size_t size)
{
	//__terminal_putchar('<');
	//__terminal_putchar(' ');
	for (size_t i = 0; i < size; i++)
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
			case '\n':
				terminal_column = window.anchor+1;
				__terminal_scroll();
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