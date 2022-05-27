#include <terminal.h>
#include <stdarg.h>
#include <screen.h>
/*
	Main code for terminal output mainportly used for debuggin and displaying information.
	Terminal code from:
	https://wiki.osdev.org/Meaty_Skeleton
*/

enum ASCII {
	ASCII_BLOCK = 219,
	ASCII_HORIZONTAL_LINE = 205,
	ASCII_VERTICAL_LINE = 186,
	ASCII_DOWN_INTERSECT = 203
};

static const char newline = '\n';

#define TERMINAL_START (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)
#define TERMINAL_WIDTH (SCREEN_WIDTH/3)
#define MEMORY_WIDTH (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)

 /*
	TERMINAL
*/
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

/* TERMINAL PROTOTYPES */
void __terminal_ui_text();
void __terminal_draw_lines();
void terminal_clear();
void terminal_initialize(void);
static void __terminal_scroll();
void terminal_setcolor(uint8_t color);
void __terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void twrite(const char* data);

/**
 * Adds the lower UI text to the screen and draws lines.
 * @return void
 */
void __terminal_ui_text()
{
	terminal_setcolor(VGA_COLOR_LIGHT_BLUE);
	const char* term_str = " TERMINAL ";
	for (size_t i = 0; i < strlen(term_str); i++)
	{
		scrput(i+1, TERMINAL_START, term_str[i], terminal_color);
	}

	const char* mem_str = " MEMORY ";
	for (size_t i = 0; i < strlen(mem_str); i++)
	{
		scrput(i+MEMORY_WIDTH, TERMINAL_START, mem_str[i], terminal_color);
	}

	const char* exm_str = " EXAMPLE ";
	for (size_t i = 0; i < strlen(exm_str); i++)
	{
		scrput(i+(MEMORY_WIDTH+(SCREEN_WIDTH/6)), TERMINAL_START, exm_str[i], terminal_color);
	}

}

/**
 * Draws a visual of how much memory is used.
 * 
 * @param int used, how much memory is used.
 * @return void
 */
void draw_mem_usage(int used)
{	
	int _used 		= used % (SCREEN_WIDTH/6);	
	size_t mem_size 	= (SCREEN_HEIGHT-TERMINAL_START);
	size_t mem_used 	= 1+((SCREEN_WIDTH/6)-_used);
	size_t mem_free 	= (SCREEN_WIDTH/6)-_used;

	/* Fill red with the used memory */
	for (size_t x = 1; x < mem_size; x++) {
		for (size_t y = mem_used; y < (SCREEN_WIDTH/6); y++) {
			scrput(((MEMORY_WIDTH)-2+y), TERMINAL_START+x, 176, VGA_COLOR_LIGHT_RED);
		}
	}

	/* Fill green with the free memory. */
	for (size_t x = 1; x < mem_size; x++) {
		for (size_t y = 1; y < mem_free; y++) {
			scrput(((MEMORY_WIDTH)-2+y), TERMINAL_START+x, 176, VGA_COLOR_LIGHT_GREEN);
		}
	}

	terminal_setcolor(VGA_COLOR_LIGHT_GREY);
}

/**
 * Draws lines on the screen separating terminal, memory and example.
 * 
 * @return void
 */
void __terminal_draw_lines()
{
	for (size_t x = 0; x < SCREEN_WIDTH; x++) {
		scrput(x,TERMINAL_START, ASCII_HORIZONTAL_LINE, terminal_color);
	}

	/* Vertical lines for memory */
	terminal_setcolor(VGA_COLOR_LIGHT_GREY);
	for (size_t x = 0; x < SCREEN_HEIGHT; x++) {
		scrput(MEMORY_WIDTH-2, TERMINAL_START+x, ASCII_VERTICAL_LINE, terminal_color);
	}
	scrput(MEMORY_WIDTH-2, TERMINAL_START, ASCII_DOWN_INTERSECT, terminal_color);

	/* Vertical lines for example */
	for (size_t x = 0; x < SCREEN_HEIGHT; x++) {
		scrput(((MEMORY_WIDTH+(SCREEN_WIDTH/6))-2), TERMINAL_START+x, ASCII_VERTICAL_LINE, terminal_color);
	}
	scrput(((MEMORY_WIDTH+(SCREEN_WIDTH/6))-2), TERMINAL_START, ASCII_DOWN_INTERSECT, terminal_color);
}

/**
 * Clears the terminal window.
 * @return void
 */
void terminal_clear()
{	
	/* Clears the terminal window */
	for (size_t y = TERMINAL_START+1; y < SCREEN_HEIGHT; y++)
	{
		for (size_t x = 0; x < TERMINAL_WIDTH; x++)
		{
			scrput(x, y, ' ', terminal_color);
		}
	}
}

/**
 * Defines the terminal area and clears screen.
 * @return void
 */
void init_terminal(void)
{
	terminal_row = TERMINAL_START+1;
	terminal_column = 0;
	terminal_color = VGA_COLOR_LIGHT_GREY;
	terminal_buffer = VGA_MEMORY;

	/* Clears screen */
	scr_clear();

	scrwrite(20, 0, "   ___             ______                       ", VGA_COLOR_MAGENTA);
	scrwrite(20, 1, "  |_  |            | ___ \\                      ", VGA_COLOR_MAGENTA);
	scrwrite(20, 2, "    | | ___   ___  | |_/ / __ _ _   _  ___ _ __ ", VGA_COLOR_MAGENTA);
	scrwrite(20, 3, "    | |/ _ \\ / _ \\ | ___ \\/ _` | | | |/ _ | '__|", VGA_COLOR_MAGENTA);
	scrwrite(20, 4, "/\\__/ | (_) |  __/ | |_/ | (_| | |_| |  __| |   ", VGA_COLOR_MAGENTA);
	scrwrite(20, 5, "\\____/ \\___/ \\___| \\____/ \\__,_|\\__, |\\___|_|   ", VGA_COLOR_MAGENTA);
	scrwrite(20, 6, "                                 __/ |          ", VGA_COLOR_MAGENTA);
	scrwrite(20, 7, "                                |___/           ", VGA_COLOR_MAGENTA);

	__terminal_draw_lines();
	__terminal_ui_text();

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
	scr_scroll(TERMINAL_WIDTH, TERMINAL_START);
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
void __terminal_putchar(char c)
{
	unsigned char uc = c;

	if(c == newline)
	{
		terminal_column = 0;
		if(terminal_row < SCREEN_HEIGHT-1)
		{
			terminal_row += 1;
			return;
		}
		__terminal_scroll();
		return;
	}
	
	if (terminal_column == TERMINAL_WIDTH+6)
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
	__terminal_putchar('<');
	__terminal_putchar(' ');
	for (size_t i = 0; i < size; i++)
		__terminal_putchar(data[i]);
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