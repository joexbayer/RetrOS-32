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
#include <gfx/gfxlib.h>
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

    //CLI();
    int i, j;
    for (j = y; j < (y+height); j++)
        for (i = x; i < (x+width); i++)
            putpixel(current_running->gfx_window->inner, j, i, color, current_running->gfx_window->height-18);
    //STI();

    //current_running->gfx_window->changed = 1;
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

    //CLI();
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {

                if((x)+i < 0 || (y)+l < 0 || (x)+i > current_running->gfx_window->width || (y)+l > current_running->gfx_window->height)
                    continue;
                putpixel(current_running->gfx_window->inner, (y)+l, (x)+i, color, current_running->gfx_window->height-18);
            }
        }
    }
    //STI();

	//dbgprintf("[GFX] %s put %c\n", current_running->name, c);
    //current_running->gfx_window->changed = 1;

    return 0;
}

void gfx_commit()
{
	current_running->gfx_window->changed = 1;
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

void gfx_set_color(unsigned char c)
{
	current_running->gfx_window->color = c;
}

void gfx_line(int x, int y, int length, int option, int color)
{

	//CLI();
	switch (option)
	{
	case GFX_LINE_INNER_VERTICAL:
		for (int i = y; i < (y+length); i++){
			putpixel(current_running->gfx_window->inner, i, x, VESA8_COLOR_LIGHT_GRAY4, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, i, x+1, VESA8_COLOR_GRAY4, current_running->gfx_window->height-18);
		}
		break;
	
	case GFX_LINE_INNER_HORIZONTAL:
		for (int i = x; i < (x+length); i++){
			putpixel(current_running->gfx_window->inner, y+1, i, VESA8_COLOR_LIGHT_GRAY4, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, y, i, VESA8_COLOR_GRAY4, current_running->gfx_window->height-18);
		}
		break;

	case GFX_LINE_OUTER_VERTICAL:
		for (int i = y; i < (y+length); i++){
			putpixel(current_running->gfx_window->inner, i, x, VESA8_COLOR_GRAY4, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, i, x+1, VESA8_COLOR_LIGHT_GRAY4, current_running->gfx_window->height-18);
		}
		break;

	case GFX_LINE_OUTER_HORIZONTAL:
		for (int i = x; i < (x+length); i++){
			putpixel(current_running->gfx_window->inner, y+1, i, VESA8_COLOR_GRAY4, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, y, i, VESA8_COLOR_LIGHT_GRAY4, current_running->gfx_window->height-18);
		}
		break;

	case GFX_LINE_VERTICAL:
		for (int i = y; i < (y+length); i++)
			putpixel(current_running->gfx_window->inner, i, x, color, current_running->gfx_window->height-18);
		break;

	case GFX_LINE_HORIZONTAL:
		for (int i = x; i < (x+length); i++)
        	putpixel(current_running->gfx_window->inner, y, i, color, current_running->gfx_window->height-18);
		break;


	default:
		break;
	}
	//STI();
}

void gfx_inner_box(int x, int y, int w, int h, int fill)
{	
	//CLI();
	if(fill)
    	gfx_draw_rectangle(x, y, w, h, VESA8_COLOR_LIGHT_GRAY3);

    gfx_line(x, y, w, GFX_LINE_HORIZONTAL, VESA8_COLOR_DARK_GRAY2);
    gfx_line(x, y+h, w, GFX_LINE_HORIZONTAL, VESA8_COLOR_LIGHT_GRAY1);

    gfx_line(x, y, h,GFX_LINE_VERTICAL, VESA8_COLOR_DARK_GRAY2);
    gfx_line(x+w, y, h, GFX_LINE_VERTICAL, VESA8_COLOR_LIGHT_GRAY1);

	//STI();
}



void gfx_outer_box(int x, int y, int w, int h, int fill)
{
	//CLI();
	if(fill)
    	gfx_draw_rectangle(x, y, w, h, VESA8_COLOR_LIGHT_GRAY3);

    gfx_line(x, y, w, GFX_LINE_HORIZONTAL, VESA8_COLOR_LIGHT_GRAY1);
    gfx_line(x, y+h, w, GFX_LINE_HORIZONTAL,VESA8_COLOR_DARK_GRAY2);

    gfx_line(x, y, h, GFX_LINE_VERTICAL,VESA8_COLOR_LIGHT_GRAY1);
    gfx_line(x+w, y, h,GFX_LINE_VERTICAL, VESA8_COLOR_DARK_GRAY2);

	//STI();
}

void gfx_button(int x, int y, int w, int h, char* text)
{
	gfx_outer_box(x, y, w, h, 0);
	gfx_draw_text(x+2, y+2, text, VESA8_COLOR_BLACK);
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