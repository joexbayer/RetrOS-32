#ifndef __GFXLIB_H
#define __GFXLIB_H

#include <gfx/events.h>
#include <gfx/window.h>

enum GFX_LINE_ATTRIBUTES {
    GFX_LINE_INNER_VERTICAL,
    GFX_LINE_INNER_HORIZONTAL,
    GFX_LINE_OUTER_VERTICAL,
    GFX_LINE_OUTER_HORIZONTAL,
    GFX_LINE_VERTICAL,
    GFX_LINE_HORIZONTAL
};

int gfx_event_loop(struct gfx_event* event);
int gfx_push_event(struct gfx_window* w, struct gfx_event* e);

void gfx_line(int x, int y, int length, int option, int color);

int kernel_gfx_draw_rectangle(int x, int y, int width, int height, unsigned char color);
int kernel_gfx_draw_char(int x, int y, unsigned char c, unsigned char color);
int kernel_gfx_draw_text(int x, int y, char* str, unsigned char color);
int kernel_gfx_draw_format_text(int x, int y, unsigned char color, char* fmt, ...);

void kernel_gfx_draw_circle(int xc, int yc, int r, unsigned  char color);
void kernel_gfx_draw_line(int x0, int y0, int x1, int y1, unsigned  char color);

int gfx_get_window_width();
int gfx_get_window_height();
int gfx_window_reize(int width, int height);

int kernel_gfx_set_title(char* title);

void kernel_gfx_set_position(int x, int y);

void gfx_inner_box(int x, int y, int w, int h, int fill);
void gfx_outer_box(int x, int y, int w, int h, int fill);

void gfx_button(int x, int y, int w, int h, char* text);

void gfx_commit();

#endif // !__GFXLIB_H