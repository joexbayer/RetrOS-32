/**
 * @file graphics.c
 * @author Joe Bayer (joexbayer)
 * @brief Userspace graphics library.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <lib/graphics.h>
#include <lib/syscall.h>
#include <libc.h>
#include <args.h>

#ifdef __cplusplus
extern "C" {
#endif

int gfx_draw_char(int x, int y, char data, unsigned char color)
{
    struct gfx_char c = {
        .color = color,
        .data = data,
        .x = x,
        .y = y
    };
    gfx_draw_syscall(GFX_DRAW_CHAR_OPT, &c, 0);

    return 0;
}

int gfx_draw_pixel(int x, int y, unsigned char color)
{
	struct gfx_pixel p = {
		.x = x,
		.y = y,
		.color = color
	};

	gfx_draw_syscall(GFX_DRAW_PIXEL, &p, 0);

	return 0;
}

int gfx_draw_circle(int x, int y, int r, unsigned char color, char fill)
{
	struct gfx_circle c = {
		.x = x,
		.y = y,
		.r = r,
		.fill = fill,
		.color = color
	};

	gfx_draw_syscall(GFX_DRAW_CIRCLE_OPT, &c, 0);

	return 0;
}

int gfx_draw_line(int x0, int y0, int x1, int y1, unsigned char color)
{
	struct gfx_line line = {
		.x0 = y0,
		.y0 = x0,
		.y1 = x1,
		.x1 = y1,
		.color = color
	};

	gfx_draw_syscall(GFX_DRAW_LINE_OPT, &line, 0);

	return 0;
}

int gfx_draw_rectangle_rgb(int x, int y, int width, int height, unsigned char color)
{
	struct gfx_rectangle rect = {
		.color = color,
		.x = x,
		.y = y,
		.width = width,
		.height = height,
		.palette = GFX_RGB
	};

	gfx_draw_syscall(GFX_DRAW_RECTANGLE_OPT, &rect, 0);

	return 0;
}

int gfx_draw_rectangle(int x, int y, int width, int height, unsigned char color)
{
    struct gfx_rectangle rect = {
		.color = color,
		.x = x,
		.y = y,
		.width = width,
		.height = height,
		.palette = GFX_VGA
	};

    gfx_draw_syscall(GFX_DRAW_RECTANGLE_OPT, &rect, 0);

    return 0;
}

int gfx_draw_text(int x, int y, const char* text, unsigned char color)
{
    struct gfx_char c = {
        .color = color,
        .data = 0,
        .x = x,
        .y = y
    };

    int len = strlen(text);
    for (int i = 0; i < len; i++)
    {
        c.data = text[i];
        gfx_draw_syscall(GFX_DRAW_CHAR_OPT, &c, 0);
        c.x += 8;
    }
    
    return 0;
}

int gfx_get_event(struct gfx_event* event, gfx_event_flag_t flags)
{
	return gfx_draw_syscall(GFX_EVEN_LOOP_OPT, event, flags);
}

#define GFX_MAX_FMT 50
int gfx_draw_format_text(int x, int y, unsigned char color, const char* fmt, ...)
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
						gfx_draw_text(x+(x_offset*8), y, str, color);
						x_offset += strlen(str);
						break;
                    case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);

                        if(strlen(str) < 4){
                            int pad = 4-strlen(str);
                            for (int i = 0; i < pad; i++){
                                gfx_draw_char(x+(x_offset*8), y, '0', color);
                                x_offset++;
                            }
                        }
						gfx_draw_text(x+(x_offset*8), y, str, color);
						x_offset += strlen(str);
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						gfx_draw_text(x+(x_offset*8), y, str, color);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						gfx_draw_text(x+(x_offset*8), y, str_arg, color);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						gfx_draw_char(x+(x_offset*8), y, char_arg, color);
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
				gfx_draw_char(x+(x_offset*8), y, *fmt, color);
				x_offset++;
                written++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}

#ifdef __cplusplus
}
#endif
