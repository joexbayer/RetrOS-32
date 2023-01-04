#include <gfx/window.h>
#include <util.h>
#include <gfx/component.h>
#include <gfx/composition.h>
#include <colors.h>
#include <serial.h>
#include <vesa.h>

void gfx_draw_window(uint8_t* buffer, struct gfx_window* window)
{
    /* Draw main frame of window with title bar and borders */
    vesa_fillrect(buffer, window->x, window->y, window->width, window->height, GFX_WINDOW_BG_COLOR);

    /* contour colors */
    vesa_line_vertical(buffer,window->x, window->y, window->height, VESA8_COLOR_LIGHT_GRAY1);
    vesa_line_vertical(buffer,window->x+window->width, window->y, window->height, VESA8_COLOR_DARK_GRAY2);

    vesa_line_horizontal(buffer,window->x, window->y, window->width, VESA8_COLOR_LIGHT_GRAY1);
    vesa_line_horizontal(buffer,window->x, window->y+window->height, window->width, VESA8_COLOR_DARK_GRAY2);

    /* header color */
    vesa_fillrect(buffer, window->x+2, window->y+2, window->width-4, GFX_WINDOW_TITLE_HEIGHT, window->in_focus ? VESA8_COLOR_DARK_BLUE : VESA8_COLOR_DARK_GRAY1);

    vesa_write_str(buffer, window->x+4, window->y+4, window->name, VESA8_COLOR_WHITE);
    if(window->inner != NULL){
        int i, j, c = 0;
        for (j = window->x+2; j < (window->x+window->width-2); j++)
            for (i = window->y+16; i < (window->y+window->height-2); i++)
                putpixel(buffer, j, i, window->inner[c++], vbe_info->pitch);
    }

}   

void gfx_default_click(struct gfx_window* window, int x, int y)
{
    dbgprintf("[GFX WINDOW] Clicked %s\n", window->name);

    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        dbgprintf("[GFX WINDOW] Clicked %s title\n", window->name);
    }
}

void gfx_default_hover(struct gfx_window* window, int x, int y)
{
    dbgprintf("[GFX] %s: hover event.\n", window->name);
    if(window->is_moving.state == GFX_WINDOW_MOVING){

        if(window->x - (window->is_moving.x - x) < 0 || window->x - (window->is_moving.x - x) + window->width > 640)
            return;
        
        if(window->y - (window->is_moving.y - y) < 0 || window->y - (window->is_moving.y - y) + window->height > 480)
            return;

        window->x -= window->is_moving.x - x;
        window->y -= window->is_moving.y - y;

        window->is_moving.x = x;
        window->is_moving.y = y;
    }
}

void gfx_default_mouse_down(struct gfx_window* window, int x, int y)
{
    dbgprintf("[GFX] %s: mousedown event.\n", window->name);
    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        window->is_moving.state = GFX_WINDOW_MOVING;
        window->is_moving.x = x;
        window->is_moving.y = y;
        dbgprintf("[GFX] %s: moving.\n", window->name);
    }
}

void gfx_default_mouse_up(struct gfx_window* window, int x, int y)
{
    dbgprintf("[GFX] %s: mouse up event.\n", window->name);
    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        window->is_moving.state = GFX_WINDOW_STATIC;
        window->is_moving.x = x;
        window->is_moving.y = y;
        dbgprintf("[GFX] %s: static.\n", window->name);
    }
}

struct gfx_window* gfx_new_window(int width, int height)
{
    struct gfx_window* w = (struct gfx_window*) alloc(sizeof(struct gfx_window));
    /* if a userspace program wants a window they will need to allocate inner themself. */
    w->inner = alloc(width*height);

    w->click = &gfx_default_click;
    w->mousedown = &gfx_default_mouse_down;
    w->mouseup = &gfx_default_mouse_up;
    w->hover = &gfx_default_hover;
    w->width = width + 4;
    w->height = height + 14;
    w->x = 10;
    w->y = 10;
    w->owner = current_running;
    current_running->gfx_window = w;

    w->is_moving.state = GFX_WINDOW_STATIC;
    /* Window can just use the name of the owner? */
    memcpy(w->name, current_running->name, strlen(current_running->name));
    w->in_focus = 1;

    gfx_composition_add_window(w);

    return w;
}