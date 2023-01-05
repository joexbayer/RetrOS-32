#include <pcb.h>
#include <font8.h>
#include <vesa.h>

/*
    Main inner window API
*/
int gfx_draw_rectangle(int x, int y, int width, int height, char color)
{
    if(x < 0 || y < 0 || x+width > current_running->gfx_window->width || y+height > current_running->gfx_window->height)
        return -1;

    CLI();
    //vesa_fillrect(current_running->gfx_window->inner, x, y, width, height, color);
    int i, j;
    for (j = y; j < (y+height); j++)
        for (i = x; i < (x+width); i++)
            putpixel(current_running->gfx_window->inner, j, i, color, current_running->gfx_window->width-8);
    STI();

    current_running->gfx_window->changed = 1;
    return 1;
}

int gfx_draw_char(int x, int y, char c, char color)
{
    CLI();
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {
                putpixel(current_running->gfx_window->inner, (y)+l, (x)+i, color, current_running->gfx_window->width-8);
            }
        }
    }
    STI();
    current_running->gfx_window->changed = 1;
}

int gfx_draw_text(int x, int y, char* str, char color)
{
    for (int i = 0; i < strlen(str); i++)
    {
        gfx_draw_char(x+(i*8), y, str[i], color);
    }
}

int gfx_scroll_text(int from, int to)
{
    //memcpy(current_running->gfx_window->inner+(to*8*current_running->gfx_window->width), current_running->gfx_window->inner+(from*8*current_running->gfx_window->width))
}