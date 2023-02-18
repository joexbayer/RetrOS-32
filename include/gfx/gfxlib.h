#ifndef __GFXLIB_H
#define __GFXLIB_H

enum GFX_LINE_ATTRIBUTES {
    GFX_LINE_INNER_VERTICAL,
    GFX_LINE_INNER_HORIZONTAL,
    GFX_LINE_OUTER_VERTICAL,
    GFX_LINE_OUTER_HORIZONTAL,
    GFX_LINE_VERTICAL,
    GFX_LINE_HORIZONTAL
};

void gfx_line(int x, int y, int length, int option, int color);

int gfx_draw_rectangle(int x, int y, int width, int height, char color);
int gfx_draw_char(int x, int y, char c, char color);
int __internal_gfx_draw_text(int x, int y, char* str, char color);
int gfx_draw_format_text(int x, int y, char color, char* fmt, ...);

int gfx_get_window_width();
int gfx_get_window_height();
int gfx_window_reize(int width, int height);

void gfx_inner_box(int x, int y, int w, int h, int fill);
void gfx_outer_box(int x, int y, int w, int h, int fill);

void gfx_button(int x, int y, int w, int h, char* text);

void gfx_set_color(unsigned char);

void gfx_commit();

#endif // !__GFXLIB_H