#include "terminal.h"

/*
	Main code for terminal output mainportly used for debuggin and displaying information.
	Terminal code from:
	https://wiki.osdev.org/Meaty_Skeleton
*/

/* Hardware text mode color constants. */
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

enum ASCII {
	ASCII_BLOCK = 219,
	ASCII_HORIZONTAL_LINE = 205,
	ASCII_VERTICAL_LINE = 186,
	ASCII_DOWN_INTERSECT = 203
};

static const char newline = '\n';
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static const size_t TERMINAL_START = (VGA_HEIGHT/2 + VGA_HEIGHT/5);
static const size_t TERMINAL_WIDTH = (VGA_WIDTH/4);

static const size_t MEMORY_WIDTH = (VGA_WIDTH/3)+2;

static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;


static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

 /*
	TERMINAL
*/
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_set_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x + 1;
	outportb(0x3D4, 0x0F);
	outportb(0x3D5, (uint8_t) (pos & 0xFF));
	outportb(0x3D4, 0x0E);
	outportb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
/**
 * Writes given string to terminal at specified position.
 * @param int x coordinate of screen.
 * @param int y coordinate of screen.
 * @param char* str string to print.
 * @return void
 */
void scrwrite(int x, int y, char* str)
{
	for (size_t i = 0; i < strlen(str); i++)
	{
		size_t index = x * VGA_WIDTH + (i + y);
		terminal_buffer[index] = vga_entry(str[i], terminal_color);
	}
}

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
		size_t index = TERMINAL_START * VGA_WIDTH + i+1;
		terminal_buffer[index] = vga_entry(term_str[i], terminal_color);
	}

	const char* mem_str = " MEMORY ";
	for (size_t i = 0; i < strlen(mem_str); i++)
	{
		size_t index = TERMINAL_START * VGA_WIDTH + i+MEMORY_WIDTH;
		terminal_buffer[index] = vga_entry(mem_str[i], terminal_color);
	}

	const char* exm_str = " EXAMPLE ";
	for (size_t i = 0; i < strlen(exm_str); i++)
	{
		size_t index = TERMINAL_START * VGA_WIDTH + i+(MEMORY_WIDTH+MEMORY_WIDTH);
		terminal_buffer[index] = vga_entry(exm_str[i], terminal_color);
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
	int _used 		= used % MEMORY_WIDTH;	
	size_t mem_size 	= (VGA_HEIGHT-TERMINAL_START);
	size_t mem_used 	= 1+(MEMORY_WIDTH-_used);
	size_t mem_free 	= MEMORY_WIDTH-_used;

	/* Fill red with the used memory */
	terminal_setcolor(VGA_COLOR_LIGHT_RED);
	for (size_t x = 1; x < mem_size; x++) {
		for (size_t y = mem_used; y < MEMORY_WIDTH; y++) {
			const size_t index = ((MEMORY_WIDTH)-2+y)+(TERMINAL_START+x) * VGA_WIDTH;
			terminal_buffer[index] = vga_entry(176, terminal_color);
		}
	}

	/* Fill green with the free memory. */
	terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
	for (size_t x = 1; x < mem_size; x++) {
		for (size_t y = 1; y < mem_free; y++) {

			const size_t index = ((MEMORY_WIDTH)-2+y)+(TERMINAL_START+x) * VGA_WIDTH;
			terminal_buffer[index] = vga_entry(176, terminal_color);
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
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		const size_t index = TERMINAL_START * VGA_WIDTH + x;
		terminal_buffer[index] = vga_entry(ASCII_HORIZONTAL_LINE, terminal_color);
	}

	/* Vertical lines for memory */
	terminal_setcolor(VGA_COLOR_LIGHT_GREY);
	for (size_t x = 0; x < VGA_HEIGHT; x++) {
		const size_t index = (MEMORY_WIDTH-2)+(TERMINAL_START+x) * VGA_WIDTH;
		terminal_buffer[index] = vga_entry(ASCII_VERTICAL_LINE, terminal_color);
	}
	size_t index_v = (MEMORY_WIDTH-2)+(TERMINAL_START) * VGA_WIDTH;
	terminal_buffer[index_v] = vga_entry(ASCII_DOWN_INTERSECT, terminal_color);

	/* Vertical lines for example */
	for (size_t x = 0; x < VGA_HEIGHT; x++) {
		const size_t index = ((MEMORY_WIDTH+MEMORY_WIDTH)-2)+(TERMINAL_START+x) * VGA_WIDTH;
		terminal_buffer[index] = vga_entry(ASCII_VERTICAL_LINE, terminal_color);
	}
	index_v = ((MEMORY_WIDTH+MEMORY_WIDTH)-2)+(TERMINAL_START) * VGA_WIDTH;
	terminal_buffer[index_v] = vga_entry(ASCII_DOWN_INTERSECT, terminal_color);
}

/**
 * Defines the terminal area and clears screen.
 * @return void
 */
void terminal_initialize(void)
{
	terminal_row = TERMINAL_START+1;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;

	/* Clears screen */
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}

	__terminal_draw_lines();
	__terminal_ui_text();

	terminal_setcolor(VGA_COLOR_WHITE);
	terminal_set_cursor(0, 0); 
}

/**
 * Used to remove old messages and keep newest.
 * Scrolls down the terminal by moving all lines 1 step up.
 * @return void
 */
static void __terminal_scroll()
{	
	/* Move all lines up, overwriting the oldest message. */
	for (size_t y = TERMINAL_START+1; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < TERMINAL_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			const size_t index_b = (y+1) * VGA_WIDTH + x;
			terminal_buffer[index] = terminal_buffer[index_b];
		}
	}

	/* clear last line of terminal */
	for (size_t x = 0; x < TERMINAL_WIDTH; x++)
	{
		const size_t index = VGA_HEIGHT * VGA_WIDTH + x;
		terminal_buffer[index] = vga_entry(' ', terminal_color);
	}
}

/**
 * Clears the terminal window.
 * @return void
 */
void terminal_clear()
{	
	/* Clears the terminal window */
	for (size_t y = TERMINAL_START+1; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < TERMINAL_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
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
 * Puts a given character to the specified screen location.
 * @param char c character to put on screen.
 * @param uint8_t color of character
 * @param size_t x coordinate
 * @param size_t y coordinate
 * @return void
 */
static void __terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

/**
 * Puts a given character to the terminal.
 * @param char c character to put on screen.
 * @return void
 */
static void __terminal_putchar(char c)
{
	unsigned char uc = c;

	if(c == newline)
	{
		terminal_column = 0;
		if(terminal_row < VGA_HEIGHT)
		{
			terminal_row += 1;
			return;
		}
		__terminal_scroll();
		return;
	}
	
	__terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == TERMINAL_WIDTH)
	{
		terminal_column = 0;
	}
}
 
/**
 * Writes the given string to the terminal with terminal_putchar
 * @param char* data to print to screen
 * @param size_t size of data
 * @return void
 */
void terminal_write(const char* data, size_t size)
{
	__terminal_putchar('>');
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