#ifndef __GRAPHICS_H
#define __GRAPHICS_H

/**
 * @brief Library version of kernel gfxlib
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#include <lib/syscall.h>
#include <args.h>


#endif

#include <gfx/events.h>

enum gfx_lib_api_options {
    GFX_DRAW_CHAR_OPT,
    GFX_DRAW_TEXT_OPT,
    GFX_PUT_CHAR_OPT,
    GFX_DRAW_RECTANGLE_OPT,
    GFX_DRAW_CIRCLE_OPT,
    GFX_DRAW_LINE_OPT,
    GFX_EVEN_LOOP_OPT,
    GFX_DRAW_PIXEL
};

struct gfx_pixel {
    int x, y;
    unsigned char color;
};

struct gfx_rectangle {
    int x, y, width, height;
    unsigned char color;
};

struct gfx_circle {
    int x, y, r;
    char fill;
    unsigned char color;
};

struct gfx_line {
    int x0, y0, x1, y1;
    unsigned char color;
};

#define GFX_TEXT_MAX 100
struct gfx_text {
    int x, y;
    char data[GFX_TEXT_MAX];
    unsigned char color;
};

struct gfx_char {
    int x, y;
    char data;
    unsigned char color;
};

int gfx_draw_text(int x, int y, const char* text, unsigned char color);
int gfx_draw_format_text(int x, int y, unsigned char color, const char* fmt, ...);
int gfx_draw_char(int x, int y, char data, unsigned char color);
int gfx_draw_rectangle(int x, int y, int width, int height, unsigned char color);
int gfx_draw_circle(int x, int y, int r, unsigned char color, char fill);
int gfx_draw_line(int x0, int y0, int x1, int y1, unsigned char color);
int gfx_get_event(struct gfx_event*, gfx_event_flag_t flags);
int gfx_draw_pixel(int x, int y, unsigned char color);

#ifdef __cplusplus
/**
 * Base class for a simple window.
 * Creates a window and sets title.
 * More to come..,
 */
class Window {
public:
	Window(int width, int height, const char* name, int flags) {
		gfx_create_window(width, height, flags);
		gfx_set_title(name);
	}

    void drawRect(int x, int y, int width, int height, unsigned char color)
    {
        gfx_draw_rectangle(x, y, width, height, color);
    }

    void drawCircle(int x, int y, int r, unsigned char color, char fill)
    {
        gfx_draw_circle(x, y, r, color, fill);
    }

    void drawLine(int x0, int y0, int x1, int y1, unsigned char color)
    {
        gfx_draw_line(x0, y0, x1, y1, color);
    }

    void drawText(int x, int y, const char* text, unsigned char color)
    {
        gfx_draw_text(x, y, text, color);
    }

    void drawChar(int x, int y, char data, unsigned char color)
    {
        gfx_draw_char(x, y, data, color);
    }

    void drawFormatText(int x, int y, unsigned char color, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        gfx_draw_format_text(x, y, color, fmt, args);
        va_end(args);
    }

    void drawPixel(int x, int y, unsigned char color)
    {
        gfx_draw_pixel(x, y, color);
    }
	
	void setTitle(char* name){
		gfx_set_title(name);
	}

    void setHeader(const char* header)
    {
        gfx_set_header(header);
    }

private:
	
};

}
#endif


#endif /* __GRAPHICS_H */