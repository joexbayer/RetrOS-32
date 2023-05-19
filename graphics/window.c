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
#include <gfx/theme.h>
#include <gfx/gfxlib.h>
#include <colors.h>
#include <serial.h>
#include <vbe.h>
#include <kutils.h>

/**
 * @brief Draw given window to a frambuffer.
 * 
 * @param buffer 
 * @param window 
 */
void gfx_draw_window(uint8_t* buffer, struct gfx_window* window)
{

    struct gfx_theme* theme = kernel_gfx_current_theme();

    int background_color = window->in_focus ? window->color.border == 0 ? theme->window.border : window->color.border : theme->window.border;
    /* Draw main frame of window with title bar and borders */
    vesa_fillrect(buffer, window->x+8, window->y, window->width-16, 8, 18);


    /* Copy inner window framebuffer to given buffer with relativ pitch. */
    if(window->inner != NULL){
        int i, j, c = 0;
        for (j = window->y+8; j < (window->y+8+window->inner_height); j++)
            for (i = window->x+8; i < (window->x+8+window->inner_width); i++)
                putpixel(buffer, i, j, window->inner[c++], vbe_info->pitch);
    }

    vesa_line_horizontal(buffer, window->x+6, window->y+window->height-7, window->inner_width+4, background_color);
    vesa_line_horizontal(buffer, window->x+6, window->y+window->height-6, window->inner_width+4, background_color);

    /* Shadow horizontal */
    vesa_line_horizontal(buffer, window->x+6, window->y+window->height-5, window->inner_width+6, 0);
    vesa_line_horizontal(buffer, window->x+6, window->y+window->height-4, window->inner_width+6, 0);

    /* Header */
    for (int i = 0; i < (window->width/8) - 2; i++){
        vesa_put_box(buffer, 80, window->x+8+(i*8), window->y-4, background_color);
        vesa_put_box(buffer, 80, window->x+8+(i*8), window->y+2, background_color);
        vesa_put_box(buffer, 80, window->x+8+(i*8), window->y-2, background_color);

        //vesa_put_box(buffer, 0, window->x+8+(i*8), window->y-4+window->height-8, background_color);;
    }

    vesa_line_vertical(buffer, window->x+7, window->y, window->inner_height+9, background_color);
    vesa_line_vertical(buffer, window->x+6, window->y, window->inner_height+9, background_color);

    vesa_line_vertical(buffer, window->x+window->width-7, window->y, window->inner_height+9, background_color);
    vesa_line_vertical(buffer, window->x+window->width-6, window->y, window->inner_height+9, background_color);

    /* Shadow vertical */
    vesa_line_vertical(buffer, window->x+window->width-5, window->y, window->inner_height+12, 0);
    vesa_line_vertical(buffer, window->x+window->width-4, window->y, window->inner_height+12, 0);


    /*
    for (int i = 0; i < (window->height/8) - 1; i++){
        vesa_put_box(buffer, 2, window->x+4, window->y+(i*8), background_color);
        vesa_put_box(buffer, 2, window->x+4-1, window->y+(i*8), background_color);

        vesa_put_box(buffer, 2, window->x-3+window->width-8, window->y+(i*8), background_color);
        vesa_put_box(buffer, 2, window->x-3+window->width-8+1, window->y+(i*8), background_color);
        vesa_put_box(buffer, 2, window->x-3+window->width-8+2, window->y+(i*8), 0);
        vesa_put_box(buffer, 2, window->x-3+window->width-8+3, window->y+(i*8), 0);
    }*/

    /* Title */
    vesa_fillrect(buffer, window->x+8, window->y, strlen(window->name)*8 + 4, 8, background_color);
    vesa_write_str(buffer, window->x+8+4, window->y, window->name, window->color.text == 0 ? theme->window.background : window->color.text);

    /* Header */
    if(window->header != 0){
        int header_position = (window->width/2) - ((strlen(window->header)*8)/2);
        vesa_fillrect(buffer, window->x+header_position, window->y, strlen(window->header)*8 + 4, 8, background_color);
        vesa_write_str(buffer, window->x+header_position+4, window->y, window->header, window->color.text == 0 ? theme->window.background : window->color.text);
    }

    /* Exit */
    vesa_fillrect(buffer,  window->x+window->width-28,  window->y, strlen("[X]")*8 - 2, 8, background_color);
    vesa_write_str(buffer, window->x+window->width-28,  window->y, "[X]", window->color.text == 0 ? theme->window.background : window->color.text);
    
    window->changed = 0;
}   

/* under construction */
void gfx_window_resize(struct gfx_window* w, int width, int height)
{
    return;
    /* Allocate new inner buffer, copy over old buffer, free old buffer, update struct */
    uint8_t* new_buffer = kalloc(width*height);
    uint8_t* old = w->inner;

    /* problem: if resizing from a larger to smaller window, what happens to data that will be "offscreen" */
    w->inner_height = height;
    w->inner_width = width;
    w->width = width + 16;
    w->height = height + 16;

    /* Copy over */
    w->inner = new_buffer;

    kfree(old);
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

    if(gfx_point_in_rectangle(window->x+window->width-20,  window->y, window->x+window->width-12, window->y+8, x, y)){
        dbgprintf("[GFX WINDOW] Clicked %s exit button\n", window->name);
        window->owner->state = ZOMBIE;
        return; 
    }
    if(gfx_point_in_rectangle(window->x, window->y, window->x+window->width, window->y+8, x, y)){
        dbgprintf("[GFX WINDOW] Clicked %s title\n", window->name);
    }
}


void gfx_default_hover(struct gfx_window* window, int x, int y)
{
    if(window->is_moving.state == GFX_WINDOW_MOVING){

        if(window->x - (window->is_moving.x - x) < 0 || window->x - (window->is_moving.x - x) + window->width > vbe_info->width)
            return;
        
        if(window->y - (window->is_moving.y - y) < 0 || window->y - (window->is_moving.y - y) + window->height > vbe_info->height)
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
    if(gfx_point_in_rectangle(window->x+8, window->y, window->x+window->width-16, window->y+10, x, y)){
        window->is_moving.state = GFX_WINDOW_MOVING;
        window->is_moving.x = x;
        window->is_moving.y = y;
    }
}

void gfx_default_mouse_up(struct gfx_window* window, int x, int y)
{
    if(gfx_point_in_rectangle(window->x+8, window->y, window->x+window->width-16, window->y+10, x, y)){
        window->is_moving.state = GFX_WINDOW_STATIC;
        window->is_moving.x = x;
        window->is_moving.y = y;
    }
}

int kernel_gfx_window_border_color(uint8_t color)
{
    if(current_running->gfx_window == NULL) return -1;
    current_running->gfx_window->color.border = color;

    return 0;
}

int gfx_destory_window(struct gfx_window* w)
{
    if(w == NULL) return -1;

    CLI();
    
    gfx_composition_remove_window(w);

    kfree(w->inner);
    w->owner->gfx_window = NULL;
    kfree(w);

    STI();

    return 0;

}

/**
 * @brief Creates a new window and attaches it to the process who called the function.
 * Allocates the inner buffer in the processes personal memory.
 * @param width 
 * @param height 
 * @return struct gfx_window* 
 */
struct gfx_window* gfx_new_window(int width, int height)
{
    if(current_running->gfx_window != NULL)
        return current_running->gfx_window;

    struct gfx_window* w = (struct gfx_window*) kalloc(sizeof(struct gfx_window));
    if(w == NULL){
        dbgprintf("window is NULL\n");
        return NULL;
    }

    w->inner = kalloc(width*height);
    if(w->inner == NULL){
        dbgprintf("Inner window is NULL\n");
        return NULL;
    }
    memset(w->inner, 0, width*height);
    memset(w->events.list, 0, sizeof(struct gfx_event)*GFX_MAX_EVENTS);

    w->click = &gfx_default_click;
    w->mousedown = &gfx_default_mouse_down;
    w->mouseup = &gfx_default_mouse_up;
    w->hover = &gfx_default_hover;
    w->inner_height = height;
    w->inner_width = width;
    w->width = width + 16;
    w->height = height + 16;
    w->pitch = w->inner_width;
    w->x = 16;
    w->y = 16;
    w->owner = current_running;
    current_running->gfx_window = w;
    w->changed = 1;
    w->color.border = 0;
    w->color.header = 0;
    w->color.text = 0;
    w->spinlock = 0;
    
    w->events.head = 0;
    w->events.tail = 0;

    w->is_moving.state = GFX_WINDOW_STATIC;
    /* Window can just use the name of the owner? */
    memcpy(w->name, current_running->name, strlen(current_running->name)+1);
    memset(w->header, 0, GFX_MAX_WINDOW_NAME_SIZE);
    w->in_focus = 0;

    dbgprintf("[Window] Created new window for %s at 0x%x: inner 0x%x (total %x - %x)\n", current_running->name, w, w->inner, sizeof(struct gfx_window), width*height);

    gfx_composition_add_window(w);

    return w;
}