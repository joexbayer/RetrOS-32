#ifndef __GRAPHICS_H
#define __GRAPHICS_H

/**
 * @brief Library version of kernel gfxlib
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <gfx/events.h>

enum gfx_lib_api_options {
    GFX_DRAW_CHAR_OPT,
    GFX_DRAW_TEXT_OPT,
    GFX_PUT_CHAR_OPT,
    GFX_DRAW_RECTANGLE_OPT,
    GFX_DRAW_CIRCLE_OPT,
    GFX_DRAW_LINE_OPT,
    GFX_EVEN_LOOP_OPT
};

struct gfx_rectangle {
    int x, y, width, height;
    unsigned char color;
};

struct gfx_circle {
    int x, y, r;
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

int gfx_draw_text(int x, int y, char* text, unsigned char color);
int gfx_draw_format_text(int x, int y, char color, char* fmt, ...);
int gfx_draw_char(int x, int y, char data, unsigned char color);
int gfx_draw_rectangle(int x, int y, int width, int height, unsigned char color);
int gfx_draw_circle(int x, int y, int r, unsigned char color);
int gfx_draw_line(int x0, int y0, int x1, int y1, unsigned char color);
int gfx_get_event(struct gfx_event*);

#ifdef __cplusplus
}
#endif


#endif /* __GRAPHICS_H */