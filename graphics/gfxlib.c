/**
 * @file gfxlib.c
 * @author Joe Bayer (joexbayer)
 * @brief GFX library for rending to a framebuffer.
 * @version 0.1
 * @date 2023-01-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <pcb.h>
#include <font8.h>
#include <vbe.h>
#include <stdarg.h>
#include <vesa.h>


int gfx_get_window_width()
{
    return current_running->gfx_window->width;
}

int gfx_get_window_height()
{
    return current_running->gfx_window->height;
}

int gfx_window_reize(int width, int height)
{
    if(width > vbe_info->width || height > vbe_info->height || width < 1 || height < 1)
        return -1;

    CLI();

    current_running->gfx_window->width = width;
    current_running->gfx_window->height = height;

    STI();
	return 0;
}

/**
 * @brief Draws a rectangle onto the inner framebufferfor currently running process.
 * 
 * @param x coordinate
 * @param y coordiante
 * @param width of rectangle
 * @param height of rectangle
 * @param color
 * @return int 0 on success, less than 0 on error
 */
int gfx_draw_rectangle(int x, int y, int width, int height, char color)
{
    if(current_running->gfx_window == NULL)
        return -1;

    if(x < 0 || y < 0 || x+width > current_running->gfx_window->width || y+height > current_running->gfx_window->height)
        return -2;

    CLI();
    int i, j;
    for (j = y; j < (y+height); j++)
        for (i = x; i < (x+width); i++)
            putpixel(current_running->gfx_window->inner, j, i, color, current_running->gfx_window->height-18);
    STI();

    current_running->gfx_window->changed = 1;
    return 0;
}

/**
 * @brief Draws a character to the framebuffer of currently running process.
 * 
 * @param x coordinate
 * @param y coordiante
 * @param c character
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int gfx_draw_char(int x, int y, char c, char color)
{

    if(current_running->gfx_window == NULL)
        return -1;

    CLI();
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {

                if((x)+i < 0 || (y)+l < 0 || (x)+i > current_running->gfx_window->width || (y)+l > current_running->gfx_window->height)
                    continue;
                putpixel(current_running->gfx_window->inner, (y)+l, (x)+i, color, current_running->gfx_window->height-18);
            }
        }
    }
    STI();

	//dbgprintf("[GFX] %s put %c\n", current_running->name, c);
    current_running->gfx_window->changed = 1;

    return 0;
}

/**
 * @brief gfx_draw_char wrapper for strings
 * 
 * @param x 
 * @param y 
 * @param str string to write.
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int gfx_draw_text(int x, int y, char* str, char color)
{
    if(current_running->gfx_window == NULL)
        return -1;

    for (int i = 0; i < strlen(str); i++)
    {
        gfx_draw_char(x+(i*8), y, str[i], color);
    }

    return 0;
}


#define GFX_MAX_FMT 50
int gfx_draw_format_text(int x, int y, char color, char* fmt, ...)
{
	va_list args;

	int x_offset = 0;
	int written = 0;
	char str[GFX_MAX_FMT];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt)
		{
			case '%':
				memset(str, 0, GFX_MAX_FMT);
				switch (*(fmt+1))
				{
					case 'd':
					case 'i': ;
						num = va_arg(args, int);
						itoa(num, str);
						gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
                    case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                gfx_draw_char(' ', x+(x_offset*PIXELS_PER_CHAR), y, color);
                                x_offset++;
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str_arg, color);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						gfx_draw_char(char_arg, x+(x_offset*PIXELS_PER_CHAR), y, color);
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
				gfx_draw_char(x+(x_offset*PIXELS_PER_CHAR), y, *fmt, color);
				x_offset++;
                written++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}