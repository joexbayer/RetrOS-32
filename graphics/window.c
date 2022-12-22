#include <gfx/window.h>
#include <colors.h>
#include <vesa.h>

void gfx_draw_window(uint8_t* buffer, struct gfx_window* window)
{
    /* Draw main frame of window with title bar and borders */
    vesa_fillrect(buffer, window->x, window->y, window->width, window->height, GFX_WINDOW_BG_COLOR);

    vesa_line_vertical(buffer,window->x, window->y, window->height, VESA8_COLOR_LIGHT_GRAY1);
    vesa_line_vertical(buffer,window->x+window->width, window->y, window->height, VESA8_COLOR_DARK_GRAY2);

    vesa_line_horizontal(buffer,window->x, window->y, window->width, VESA8_COLOR_LIGHT_GRAY1);
    vesa_line_horizontal(buffer,window->x, window->y+window->height, window->width, VESA8_COLOR_DARK_GRAY2);

    vesa_fillrect(buffer, window->x+2, window->y+2, window->width-4, GFX_WINDOW_TITLE_HEIGHT, window->in_focus ? VESA8_COLOR_DARK_BLUE : VESA8_COLOR_DARK_GRAY4);

    vesa_write_str(buffer, window->x+4, window->y+4, window->name, VESA8_COLOR_WHITE);
}