#ifndef __GFXLIB_H
#define __GFXLIB_H

#include <gfx/events.h>
#include <kutils.h>
#include <gfx/window.h>

enum GFX_LINE_ATTRIBUTES {
    GFX_LINE_INNER_VERTICAL,
    GFX_LINE_INNER_HORIZONTAL,
    GFX_LINE_OUTER_VERTICAL,
    GFX_LINE_OUTER_HORIZONTAL,
    GFX_LINE_VERTICAL,
    GFX_LINE_HORIZONTAL
};

int gfx_event_loop(struct gfx_event* event, gfx_event_flag_t flags);
int gfx_push_event(struct window* w, struct gfx_event* e);

int gfx_put_icon32(unsigned char icon[], int x, int y);
int gfx_put_icon16(unsigned char icon[], int x, int y);

void gfx_line(int x, int y, int length, int option, int color);

int kernel_gfx_draw_rectangle(struct window* w, int x, int y, int width, int height, color_t color);
int kernel_gfx_draw_char(struct window* w, int x, int y, unsigned char c, unsigned char color);
int kernel_gfx_draw_text(struct window* w, int x, int y, char* str, unsigned char color);
int kernel_gfx_draw_format_text(struct window* w, int x, int y, unsigned char color, char* fmt, ...);
int kernel_gfx_draw_pixel(struct window* w, int x, int y, color_t color);
int kernel_gfx_draw_bitmap(struct window* w, int x, int y, int width, int height, uint8_t* bitmap);

void kernel_gfx_draw_circle(struct window* w, int xc, int yc, int r, unsigned char color, bool_t fill);
void kernel_gfx_draw_line(struct window* w, int x0, int y0, int x1, int y1, unsigned char color);

int gfx_get_window_width();
int gfx_get_window_height();

int kernel_gfx_set_title(char* title);
int kernel_gfx_set_header(const char* header);

int kernel_gfx_draw_contoured_box(struct window* w, int x, int y, int width, int height, color_t color);
void kernel_gfx_set_position(struct window* w, int x, int y);

void gfx_inner_box(int x, int y, int w, int h, int fill);
void gfx_outer_box(int x, int y, int w, int h, int fill);

int gfx_button(int x, int y, int width, int height, char* name);
int gfx_button_ext(int x, int y, int width, int height, char* name, color_t color);

void gfx_commit();

#endif // !__GFXLIB_H