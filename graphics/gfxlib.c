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
#include <gfx/events.h>
#include <pcb.h>
#include <font8.h>
#include <vbe.h>
#include <stdarg.h>
#include <vesa.h>
#include <serial.h>
#include <scheduler.h>

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

int gfx_push_event(struct gfx_window* w, struct gfx_event* e)
{
	//dbgprintf("Pushing event\n");

	memcpy(&w->events.list[w->events.head], e, sizeof(*e));
	w->events.head = (w->events.head + 1) % GFX_MAX_EVENTS;

	if(w->owner->running == BLOCKED)
		w->owner->running = RUNNING;

	return 0;
}

int gfx_event_loop(struct gfx_event* event)
{
	/**
	 * The gfx event loop is PCB specific,
	 * checks if there is an event if true return.
	 * Else block.
	 */

	while(1)
	{
		if(current_running->gfx_window->events.tail == current_running->gfx_window->events.head){
			block();
			continue;	
		}
	
		memcpy(event, &current_running->gfx_window->events.list[current_running->gfx_window->events.tail], sizeof(struct gfx_event));
		current_running->gfx_window->events.tail = (current_running->gfx_window->events.tail + 1) % GFX_MAX_EVENTS;
		return 0;
	}
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
int __gfx_draw_rectangle(int x, int y, int width, int height, unsigned char color)
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
int __gfx_draw_char(int x, int y, unsigned char c, unsigned char color)
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
 * @brief __gfx_draw_char wrapper for strings
 * 
 * @param x 
 * @param y 
 * @param str string to write.
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int __gfx_draw_text(int x, int y, char* str, unsigned char color)
{
    if(current_running->gfx_window == NULL)
        return -1;

    for (int i = 0; i < strlen(str); i++)
    {
        __gfx_draw_char(x+(i*8), y, str[i], color);
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
			putpixel(current_running->gfx_window->inner, i, x, COLOR_GRAY_LIGHT, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, i, x+1, COLOR_GRAY_DARK, current_running->gfx_window->height-18);
		}
		break;
	
	case GFX_LINE_INNER_HORIZONTAL:
		for (int i = x; i < (x+length); i++){
			putpixel(current_running->gfx_window->inner, y+1, i, COLOR_GRAY_LIGHT, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, y, i, COLOR_GRAY_DARK, current_running->gfx_window->height-18);
		}
		break;

	case GFX_LINE_OUTER_VERTICAL:
		for (int i = y; i < (y+length); i++){
			putpixel(current_running->gfx_window->inner, i, x, COLOR_GRAY_DARK, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, i, x+1, COLOR_GRAY_LIGHT, current_running->gfx_window->height-18);
		}
		break;

	case GFX_LINE_OUTER_HORIZONTAL:
		for (int i = x; i < (x+length); i++){
			putpixel(current_running->gfx_window->inner, y+1, i, COLOR_GRAY_DARK, current_running->gfx_window->height-18);
			putpixel(current_running->gfx_window->inner, y, i, COLOR_GRAY_LIGHT, current_running->gfx_window->height-18);
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
    	__gfx_draw_rectangle(x, y, w, h, COLOR_GRAY_DEFAULT);

    gfx_line(x, y, w, GFX_LINE_HORIZONTAL, COLOR_GRAY_DARK);
    gfx_line(x, y+h, w, GFX_LINE_HORIZONTAL, COLOR_GRAY_LIGHT);

    gfx_line(x, y, h,GFX_LINE_VERTICAL, COLOR_GRAY_DARK);
    gfx_line(x+w, y, h, GFX_LINE_VERTICAL, COLOR_GRAY_LIGHT);

	//STI();
}

int __gfx_set_title(char* title)
{
	if(strlen(title) > GFX_MAX_WINDOW_NAME_SIZE)
		return -1;
	
	memset(current_running->gfx_window->name, 0, GFX_MAX_WINDOW_NAME_SIZE);
	memcpy(current_running->gfx_window->name, title, strlen(title));

	return 0;
}

#define ABS(N) ((N<0)?(-N):(N))

/**
plotLine(x0, y0, x1, y1)
    dx = abs(x1 - x0)
    sx = x0 < x1 ? 1 : -1
    dy = -abs(y1 - y0)
    sy = y0 < y1 ? 1 : -1
    error = dx + dy
    
    while true
        plot(x0, y0)
        if x0 == x1 && y0 == y1 break
        e2 = 2 * error
        if e2 >= dy
            if x0 == x1 break
            error = error + dy
            x0 = x0 + sx
        end if
        if e2 <= dx
            if y0 == y1 break
            error = error + dx
            y0 = y0 + sy
        end if
    end while
*/

void __gfx_draw_line(int x0, int y0, int x1, int y1, unsigned char color)  
{  
    int dx, dy, sx, sy, error;
	int t1 = x1-x0;
	int t2 = y1-y0;
    dx= ABS(t1);
	sx = x0 < x1 ? 1 : -1;
    dy=-(ABS(t2));  
	sy = y0 < y1 ? 1 : -1;

	error = dx + dy;

    while(1)  
    {  
		putpixel(current_running->gfx_window->inner, x0, y0, color, current_running->gfx_window->height-18); 
		if (x0 == x1 && y0 == y1) break;
        int e2 = 2*error;

		if( e2 >= dy){
			if (x0 == x1) break;
			error = error + dy;
			x0 = x0 + sx;
		}

		if( e2 <= dx){
			if (y0 == y1) break;
			error = error + dx;
			y0 = y0 + sy;
		}
    }

	gfx_commit();
}  

void __gfx_draw_circle_helper(int xc, int yc, int x, int y, unsigned char color)
{
    putpixel(current_running->gfx_window->inner, xc+x, yc+y, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc-x, yc+y, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc+x, yc-y, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc-x, yc-y, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc+y, yc+x, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc-y, yc+x, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc+y, yc-x, color, current_running->gfx_window->height-18);
    putpixel(current_running->gfx_window->inner, xc-y, yc-x, color, current_running->gfx_window->height-18);
}
 
// using Bresenham's algorithm
void __gfx_draw_circle(int xc, int yc, int r, unsigned char color)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    __gfx_draw_circle_helper(xc, yc, x, y, color);
    while (y >= x)
    {
        // for each pixel we will
        // draw all eight pixels
         
        x++;
 
        // check for decision parameter
        // and correspondingly
        // update d, x, y
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        __gfx_draw_circle_helper(xc, yc, x, y, color);
    }

	gfx_commit();
}



void gfx_outer_box(int x, int y, int w, int h, int fill)
{
	//CLI();
	if(fill)
    	__gfx_draw_rectangle(x, y, w, h, COLOR_GRAY_DEFAULT);

    gfx_line(x, y, w, GFX_LINE_HORIZONTAL, COLOR_GRAY_LIGHT);
    gfx_line(x, y+h, w, GFX_LINE_HORIZONTAL,COLOR_GRAY_DARK);

    gfx_line(x, y, h, GFX_LINE_VERTICAL,COLOR_GRAY_LIGHT);
    gfx_line(x+w, y, h,GFX_LINE_VERTICAL, COLOR_GRAY_DARK);

	//STI();
}

void gfx_button(int x, int y, int w, int h, char* text)
{
	gfx_outer_box(x, y, w, h, 0);
	__gfx_draw_text(x+2, y+2, text, COLOR_BLACK);
}


#define GFX_MAX_FMT 50
int __gfx_draw_format_text(int x, int y, unsigned char color, char* fmt, ...)
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
						__gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
                    case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						__gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                __gfx_draw_char(' ', x+(x_offset*PIXELS_PER_CHAR), y, color);
                                x_offset++;
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						__gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						__gfx_draw_text(x+(x_offset*PIXELS_PER_CHAR), y, str_arg, color);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						__gfx_draw_char(char_arg, x+(x_offset*PIXELS_PER_CHAR), y, color);
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
				__gfx_draw_char(x+(x_offset*PIXELS_PER_CHAR), y, *fmt, color);
				x_offset++;
                written++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}