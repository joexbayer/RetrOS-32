#ifndef __GFXLIB_H
#define __GFXLIB_H

int gfx_draw_rectangle(int x, int y, int width, int height, char color);
int gfx_draw_char(int x, int y, char c, char color);
int gfx_draw_text(int x, int y, char* str, char color);
int gfx_draw_format_text(int x, int y, char color, char* fmt, ...);

int gfx_get_window_width();
int gfx_get_window_height();
int gfx_window_reize(int width, int height);

#endif // !__GFXLIB_H