#ifndef __GFXLIB_H
#define __GFXLIB_H

int gfx_draw_rectangle(int x, int y, int width, int height, char color);
int gfx_draw_char(int x, int y, char c, char color);
int gfx_draw_text(int x, int y, char* str, char color);

#endif // !__GFXLIB_H