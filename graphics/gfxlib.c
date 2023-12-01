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
#include <args.h>
#include <vbe.h>
#include <serial.h>
#include <scheduler.h>
#include <kutils.h>

int gfx_get_window_width()
{
	return current_running->gfx_window->inner_width;
}

int gfx_get_window_height()
{
	return current_running->gfx_window->inner_height;
}

int gfx_push_event(struct window* w, struct gfx_event* e)
{
	ERR_ON_NULL(w);
	//dbgprintf("Pushing event %d (head %d): data %d, data2 %d\n", e->event, w->events.head, e->data, e->data2);

	SPINLOCK(w,  {
		memcpy(&w->events.list[w->events.head], e, sizeof(*e));
		w->events.head = (w->events.head + 1) % GFX_MAX_EVENTS;
	});

	/* TODO: Check for a block reason */
	if(w->owner->state == BLOCKED) w->owner->state = RUNNING;

	return 0;
}

/**
 drawRect(x, y, width, height, 30);
        
        // Top Inner Border
        drawRect(x, y, width-1, 1, 31);
        
        // Left Inner Border
        drawRect(x, y, 1, height, 31);
        
        // Right Inner Border
        drawRect(x+width-1, y, 1, height, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        
        // Bottom Inner Borders
        drawRect(x, y+height-1, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        drawRect(x, y+height, width-1, 1, 31);
*/
int gfx_draw_contoured_box(int x, int y, int width, int height, color_t color) 
{
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, width, height, color);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, width-1, 1, 31);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, 1, height, 31);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x+width-1, y, 1, height, COLOR_VGA_MEDIUM_DARK_GRAY+5);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y+height-1, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y+height, width-1, 1, 31);

	return 0;
}

int gfx_button(int x, int y, int width, int height, char* name)
{
	return gfx_button_ext(x, y, width, height, name, 30);
}

int gfx_button_ext(int x, int y, int width, int height, char* name, color_t color)
{
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, width, height, color);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, width-1, 1, 31);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y, 1, height, 31);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x+width-1, y, 1, height, COLOR_VGA_MEDIUM_DARK_GRAY+5);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y+height-1, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5);
	kernel_gfx_draw_rectangle(current_running->gfx_window, x, y+height, width-1, 1, 31);

	/* center the text */
	int text_width = strlen(name)*PIXELS_PER_CHAR;
	int text_height = 8;
	int text_x = x + (width/2) - (text_width/2);
	int text_y = y + (height/2) - (text_height/2);

	kernel_gfx_draw_text(current_running->gfx_window, text_x, text_y, name, 0x0);

	return 0;
}

int gfx_put_icon16(unsigned char icon[], int x, int y)
{
	for (int l = 0; l < 16; l++) {

		for (int i = 0; i < 16; i++) {
			/* for new icons we use 0xfa for transparent */
			if (icon[l*16+i] != 0xfa) {

				if((x)+i < 0 || (y)+l < 0 || (x)+i > current_running->gfx_window->inner_width || (y)+l > current_running->gfx_window->inner_height)
					continue;
				putpixel(current_running->gfx_window->inner, (x)+i, (y)+l, rgb_to_vga(icon[l*16+i]), current_running->gfx_window->pitch);
			}
		}
	}
	return 0;
}

int gfx_put_icon32(unsigned char icon[], int x, int y)
{
	for (int l = 0; l < 32; l++) {

		for (int i = 0; i < 32; i++) {
			/* for new icons we use 0xfa for transparent */
			if (icon[l*32+i] != 0xfa) {

				if((x)+i < 0 || (y)+l < 0 || (x)+i > current_running->gfx_window->inner_width || (y)+l > current_running->gfx_window->inner_height)
					continue;
				putpixel(current_running->gfx_window->inner, (x)+i, (y)+l, rgb_to_vga(icon[l*32+i]), current_running->gfx_window->pitch);
			}
		}
	}
	return 0;
}

int gfx_event_loop(struct gfx_event* event, gfx_event_flag_t flags)
{

	ERR_ON_NULL(current_running->gfx_window);
	/**
	 * The gfx event loop is PCB specific,
	 * checks if there is an event if true return.
	 * Else block.
	 */
	while(1){
		if(current_running->gfx_window->events.tail == current_running->gfx_window->events.head){
			
			if(flags & GFX_EVENT_BLOCKING){
				/* FIXME: Should not global block, perhaps window manager block queue? */
				current_running->state = BLOCKED;
				kernel_yield();
			} else {
				return -1;
			}
			continue;
		}
		
		//dbgprintf("Getting event %d (tail %d)\n", current_running->gfx_window->events.list[current_running->gfx_window->events.tail].event, current_running->gfx_window->events.tail);
		SPINLOCK(current_running->gfx_window, {
			memcpy(event, &current_running->gfx_window->events.list[current_running->gfx_window->events.tail], sizeof(struct gfx_event));
			current_running->gfx_window->events.tail = (current_running->gfx_window->events.tail + 1) % GFX_MAX_EVENTS;
		});
		return 0;
	}
}

/**
 * @brief Draws a rectangle onto the inner framebufferfor currently running process.
 * 
 * @param w window to draw to
 * @param x coordinate
 * @param y coordiante
 * @param width of rectangle
 * @param height of rectangle
 * @param color
 * @return int 0 on success, less than 0 on error
 */
int kernel_gfx_draw_rectangle(struct window* w, int x, int y, int width, int height, color_t color)
{
	ERR_ON_NULL(w);

	/* only draw rectangle inside window */
	if(x < 0 || y < 0 || x+width > w->inner_width || y+height > w->inner_height)
		return -2;

	int i, j;
	for (j = y; j < (y+height); j++)
		for (i = x; i < (x+width); i++)
			putpixel(w->inner, i, j, color, w->pitch);

	return 0;
}

int kernel_gfx_draw_pixel(struct window* w, int x, int y, color_t color)
{
	ERR_ON_NULL(w);

	/* only draw rectangle inside window */
	if(x < 0 || y < 0 || x > w->inner_width || y > w->inner_height)
		return -2;

	putpixel(w->inner, x, y, color, w->pitch);

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
int kernel_gfx_draw_char(struct window* w, int x, int y, unsigned char c, unsigned char color)
{

	ERR_ON_NULL(w);

	//ENTER_CRITICAL();
	for (int l = 0; l < 8; l++) {
		for (int i = 8; i >= 0; i--) {
			if (font8x8_basic[c][l] & (1 << i)) {

				if((x)+i < 0 || (y)+l < 0 || (x)+i > w->inner_width || (y)+l > w->inner_height)
					continue;
				putpixel(w->inner, (x)+i, (y)+l, color, w->pitch);
			}
		}
	}
	//LEAVE_CRITICAL();

	//dbgprintf("[GFX] %s put %c\n", current_running->name, c);
	//current_running->gfx_window->changed = 1;

	return 0;
}

void kernel_gfx_set_position(struct window* w, int x, int y)
{
	w->x = x;
	w->y = y;
}

void gfx_commit()
{
	current_running->gfx_window->changed = 1;
}

/**
 * @brief kernel_gfx_draw_char wrapper for strings
 * 
 * @param x 
 * @param y 
 * @param str string to write.
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int kernel_gfx_draw_text(struct window* w, int x, int y, char* str, unsigned char color)
{
	ERR_ON_NULL(w);

	for (int i = 0; i < strlen(str); i++){
		kernel_gfx_draw_char(w, x+(i*8), y, str[i], color);
	}

	return 0;
}

int kernel_gfx_set_title(char* title)
{
	if(strlen(title) > GFX_MAX_WINDOW_NAME_SIZE)
		return -1;
	
	memset(current_running->gfx_window->name, 0, GFX_MAX_WINDOW_NAME_SIZE);
	memcpy(current_running->gfx_window->name, title, strlen(title));

	return 0;
}

int kernel_gfx_set_header(const char* header)
{
	if(strlen(header) > GFX_MAX_WINDOW_NAME_SIZE)
		return -1;
	
	memset(current_running->gfx_window->header, 0, GFX_MAX_WINDOW_NAME_SIZE);
	memcpy(current_running->gfx_window->header, header, strlen(header));

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

void kernel_gfx_draw_line(struct window* w, int x0, int y0, int x1, int y1, unsigned char color)
{  
	if(x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0)
		return;
	
	if(x0 > w->inner_width || y0 > w->inner_height || x1 > w->inner_width || y1 > w->inner_height)
		return;

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
		putpixel(w->inner, y0, x0, color, w->pitch); 
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
}  

static void kernel_gfx_draw_circle_helper(struct window* w, int xc, int yc, int x, int y, unsigned char color, bool_t fill)
{
    // Draw the circle outline
    putpixel(w->inner, xc + x, yc + y, color, w->pitch);
    putpixel(w->inner, xc - x, yc + y, color, w->pitch);
    putpixel(w->inner, xc + x, yc - y, color, w->pitch);
    putpixel(w->inner, xc - x, yc - y, color, w->pitch);
    putpixel(w->inner, xc + y, yc + x, color, w->pitch);
    putpixel(w->inner, xc - y, yc + x, color, w->pitch);
    putpixel(w->inner, xc + y, yc - x, color, w->pitch);
    putpixel(w->inner, xc - y, yc - x, color, w->pitch);

    // Fill the circle if the flag is set
    if (fill)
    {
        for (int i = xc - x; i <= xc + x; i++) {
            for (int j = yc - y + 1; j < yc + y; j++) {
                putpixel(w->inner, i, j, color, w->pitch);
            }
        }

        for (int i = xc - y; i <= xc + y; i++) {
            for (int j = yc - x + 1; j < yc + x; j++) {
                putpixel(w->inner, i, j, color, w->pitch);
            }
        }
    }
}

// using Bresenham's algorithm
void kernel_gfx_draw_circle(struct window* w, int xc, int yc, int r, unsigned char color, bool_t fill)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    kernel_gfx_draw_circle_helper(w, xc, yc, x, y, color, fill);

    while (y >= x)
    {
        x++;

        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }

        kernel_gfx_draw_circle_helper(w, xc, yc, x, y, color, fill);
    }
}


#define GFX_MAX_FMT 50
int kernel_gfx_draw_format_text(struct window* w, int x, int y, unsigned char color, char* fmt, ...)
{
	va_list args;

	int x_offset = 0;
	int written = 0;
	char str[GFX_MAX_FMT];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {

		if(x+(x_offset*PIXELS_PER_CHAR) >= w->inner_width){
			y += 8;
			written += x_offset;
			x_offset = 0;
		}

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
						kernel_gfx_draw_text(w, x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
					case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);

						if(strlen(str) < 6){
							int pad = 6-strlen(str);
							for (int i = 0; i < pad; i++){
								kernel_gfx_draw_char(w, x+(x_offset*PIXELS_PER_CHAR), y,'0', color);
								x_offset++;
							}
						}
								dbgprintf("OFFSET %d\n", strlen(str));

						kernel_gfx_draw_text(w, x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						kernel_gfx_draw_text(w, x+(x_offset*PIXELS_PER_CHAR), y, str, color);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						kernel_gfx_draw_text(w, x+(x_offset*PIXELS_PER_CHAR), y, str_arg, color);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						kernel_gfx_draw_char(w, x+(x_offset*PIXELS_PER_CHAR), y, char_arg, color);
						x_offset++;
						break;
					default:
						break;
				}
				fmt++;
				break;
			case '\n':
				y += 8;
				written += x_offset;
				x_offset = 0;
				break;
			default:  

				kernel_gfx_draw_char(w, x+(x_offset*PIXELS_PER_CHAR), y, *fmt, color);
				x_offset++;
				written++;
			}
		fmt++;
	}
	written += x_offset;
	gfx_commit();
	return written;
}