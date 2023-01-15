/**
 * @file window.c
 * @author Joe Bayer (joexbayer)
 * @brief GFX Window API
 * @version 0.1
 * @date 2023-01-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <gfx/window.h>
#include <util.h>
#include <gfx/component.h>
#include <gfx/composition.h>
#include <gfx/gfxlib.h>
#include <colors.h>
#include <serial.h>
#include <vesa.h>

/**
 * @brief Draw given window to a frambuffer.
 * 
 * @param buffer 
 * @param window 
 */
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
    vesa_fillrect(buffer, window->x+2+window->width-GFX_WINDOW_TITLE_HEIGHT-4,  window->y+2, GFX_WINDOW_TITLE_HEIGHT-1, GFX_WINDOW_TITLE_HEIGHT-1, GFX_WINDOW_BG_COLOR);
    vesa_put_char(buffer, 'X', window->x+2+window->width-GFX_WINDOW_TITLE_HEIGHT-2,  window->y+4, VESA8_COLOR_BLACK);

    vesa_write_str(buffer, window->x+4, window->y+4, window->name, VESA8_COLOR_WHITE);

    /* Copy inner window framebuffer to given buffer with relativ pitch. */
    if(window->inner != NULL){
        int i, j, c = 0;
        for (j = window->x+2; j < (window->x+window->width-2); j++)
            for (i = window->y+16; i < (window->y+window->height-2); i++)
                /* FIXME: Because of the j, i order we use height as pitch in gfxlib. */
                putpixel(buffer, j, i, window->inner[c++], vbe_info->pitch);
    }
    window->changed = 0;
}   

/**
 * @brief Default handler for click event from mouse on given window.
 * 
 * @param window Window that was clicked.
 * @param x 
 * @param y 
 */
void gfx_default_click(struct gfx_window* window, int x, int y)
{
    dbgprintf("[GFX WINDOW] Clicked %s\n", window->name);

    if(gfx_point_in_rectangle(window->x+2+window->width-GFX_WINDOW_TITLE_HEIGHT-4,  window->y+2, window->x+2+window->width-GFX_WINDOW_TITLE_HEIGHT-4+GFX_WINDOW_TITLE_HEIGHT-1, window->y+2+GFX_WINDOW_TITLE_HEIGHT-1, x, y)){
        dbgprintf("[GFX WINDOW] Clicked %s exit button\n", window->name);
        window->owner->running = ZOMBIE;
        return; 
    }
    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        dbgprintf("[GFX WINDOW] Clicked %s title\n", window->name);
    }
}


void gfx_default_hover(struct gfx_window* window, int x, int y)
{
    if(window->is_moving.state == GFX_WINDOW_MOVING){

        if(window->x - (window->is_moving.x - x) < 0 || window->x - (window->is_moving.x - x) + window->width > 640)
            return;
        
        if(window->y - (window->is_moving.y - y) < 0 || window->y - (window->is_moving.y - y) + window->height > 480)
            return;

        window->x -= window->is_moving.x - x;
        window->y -= window->is_moving.y - y;

        window->is_moving.x = x;
        window->is_moving.y = y;
        
        window->changed = 1;
    }
}

void gfx_default_mouse_down(struct gfx_window* window, int x, int y)
{
    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        window->is_moving.state = GFX_WINDOW_MOVING;
        window->is_moving.x = x;
        window->is_moving.y = y;
    }
}

void gfx_default_mouse_up(struct gfx_window* window, int x, int y)
{
    if(gfx_point_in_rectangle(window->x+2, window->y+2, window->x+window->width-4, window->y+GFX_WINDOW_TITLE_HEIGHT, x, y)){
        window->is_moving.state = GFX_WINDOW_STATIC;
        window->is_moving.x = x;
        window->is_moving.y = y;
    }
}

int gfx_destory_window(struct gfx_window* w)
{
    CLI();
    
    gfx_composition_remove_window(w);

    free(w->inner);
    w->owner->window = NULL;
    kfree(w);

    STI();

    return 0;

}
/**
 * @brief 
 * 
 * @param width 
 * @param height 
 * @return struct gfx_window* 
 */
struct gfx_window* gfx_new_window(int width, int height)
{
    struct gfx_window* w = (struct gfx_window*) kalloc(sizeof(struct gfx_window));
    /* if a userspace program wants a window they will need to allocate inner themself. */
    w->inner = malloc(width*height);

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
    w->changed = 1;

    w->is_moving.state = GFX_WINDOW_STATIC;
    /* Window can just use the name of the owner? */
    memcpy(w->name, current_running->name, strlen(current_running->name));
    w->in_focus = 0;

    dbgprintf("[Window] Created new window for %s at 0x%x: inner 0x%x (total %x - %x)\n", current_running->name, w, w->inner, sizeof(struct gfx_window), width*height);

    gfx_composition_add_window(w);

    return w;
}