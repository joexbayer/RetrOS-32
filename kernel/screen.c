#include <screen.h>

uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
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
	for (size_t i = 0; i < strlen(str); i++)
	{
		size_t index = x * SCREEN_WIDTH + (i + y);
		VGA_MEMORY[index] = vga_entry(str[i], color);
	}
}

/**
 * Puts a given character to the specified screen location.
 * Index calculation = y * SCREEN_WIDTH + x;
 * @param size_t x coordinate
 * @param size_t y coordinate
 * @param char c character to put on screen.
 * @param uint8_t color of character
 * @return void
 */
void scrput(size_t x, size_t y, unsigned char c, uint8_t color)
{
	const size_t index = y * SCREEN_WIDTH + x;
	VGA_MEMORY[index] = vga_entry(c, color);
}

/**
 * Clears the entire screen.
 * @return void
 */
void scr_clear()
{
	for (size_t y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (size_t x = 0; x < SCREEN_WIDTH; x++)
		{
			const size_t index = y * SCREEN_WIDTH + x;
			VGA_MEMORY[index] = vga_entry(' ', 0);
		}
	}
}

void scr_scroll(int width, int height)
{
    /* Move all lines up, overwriting the oldest message. */
	for (size_t y = height+1; y < SCREEN_HEIGHT; y++)
	{
		for (size_t x = 0; x < width; x++)
		{
			const size_t index = y * SCREEN_WIDTH + x;
			const size_t index_b = (y+1) * SCREEN_WIDTH + x;
			VGA_MEMORY[index] = VGA_MEMORY[index_b];
		}
	}

	/* clear last line of terminal */
	for (size_t x = 0; x < width; x++)
	{
		const size_t index = SCREEN_HEIGHT * SCREEN_WIDTH + x;
		VGA_MEMORY[index] = vga_entry(' ', 0);
	}
}