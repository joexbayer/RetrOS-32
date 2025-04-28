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
#include <libc.h>
#include <gfx/component.h>
#include <gfx/composition.h>
#include <gfx/theme.h>
#include <gfx/gfxlib.h>
#include <colors.h>
#include <serial.h>
#include <vbe.h>
#include <errors.h>
#include <kutils.h>
#include <lib/icons.h>

//#define __WINDOWS_95


/* prototype window ops */
static void gfx_default_click(struct window* window, int x, int y); 
static void gfx_default_mouse_down(struct window* window, int x, int y);
static void gfx_default_mouse_up(struct window* window, int x, int y);
static void gfx_default_hover(struct window* window, int x, int y);
static void gfx_window_resize(struct window* w, int width, int height);
static int gfx_window_maximize(struct window* window);

/* default window ops struct */
static struct window_ops default_window_ops = {
    .click = &gfx_default_click,
    .mousedown = &gfx_default_mouse_down,
    .mouseup = &gfx_default_mouse_up,
    .hover = &gfx_default_hover,
    .resize = &gfx_window_resize,
    .move = &kernel_gfx_set_position,
    .destroy = &gfx_destroy_window,
    .maximize = &gfx_window_maximize
};

/* default windows draw ops */
static struct window_draw_ops default_window_draw_ops = {
    .draw = &gfx_draw_window,
    .rect = &kernel_gfx_draw_rectangle,
    .textf = &kernel_gfx_draw_format_text,
    .text = &kernel_gfx_draw_text,
    .line = &kernel_gfx_draw_line,
    .circle = &kernel_gfx_draw_circle,
    .box = &kernel_gfx_draw_contoured_box,
    .bitmap = &kernel_gfx_draw_bitmap
};

/**
 * @brief Draw given window to a frambuffer.
 * 
 * @param buffer 
 * @param window 
 */
void gfx_draw_window(uint8_t* buffer, struct window* window)
{
    struct gfx_theme* theme = kernel_gfx_current_theme();
    int padding = HAS_FLAG(window->flags, GFX_HIDE_HEADER) ? 0 : 8;
    int background_color = window->in_focus ? window->color.border == 0 ? theme->window.border : window->color.border : theme->window.border;

    color_t border = window->color.border == 0 ? theme->window.border : window->color.border;
    color_t border_light = window->color.border == 0 ? theme->window.border_accent_light : window->color.border;
    color_t border_dark = window->color.border == 0 ? theme->window.border_accent_dark : window->color.border;

    if((window->is_moving.state == GFX_WINDOW_MOVING || window->resize) && !HAS_FLAG(window->flags, GFX_IS_IMMUATABLE)){
        vesa_striped_line_horizontal(buffer, window->x, window->y, window->width, COLOR_VGA_DARKEST_GRAY, 2);
        vesa_striped_line_horizontal(buffer, window->x+2, window->y, window->width-2, COLOR_VGA_LIGHTEST_GRAY, 4);
        
        vesa_striped_line_horizontal(buffer, window->x, window->y+window->height, window->width, COLOR_VGA_DARKEST_GRAY, 2);
        vesa_striped_line_horizontal(buffer, window->x+2, window->y+window->height, window->width-2, COLOR_VGA_LIGHTEST_GRAY, 4);

        vesa_striped_line_vertical(buffer, window->x, window->y, window->height, COLOR_VGA_DARKEST_GRAY, 2);
        vesa_striped_line_vertical(buffer, window->x, window->y+2, window->height-2, COLOR_VGA_LIGHTEST_GRAY, 4);

        vesa_striped_line_vertical(buffer, window->x+window->width, window->y, window->height, COLOR_VGA_DARKEST_GRAY, 2);
        vesa_striped_line_vertical(buffer, window->x+window->width, window->y+2, window->height-2, COLOR_VGA_LIGHTEST_GRAY, 4);

        window->changed = 0;
        return;
    }

    /* Copy inner window framebuffer to given buffer with relativ pitch.  If it is NOT hidden.*/
    if(window->inner != NULL && !HAS_FLAG(window->flags, GFX_IS_HIDDEN)){
        int i, j, c = 0;
        for (j = window->y+padding; j < (window->y+padding+window->inner_height); j++)
            for (i = window->x+padding; i < (window->x+padding+window->inner_width); i++){
                if(HAS_FLAG(window->flags, GFX_IS_TRANSPARENT) && window->inner[c] == 255){
                    c++;
                    continue;
                }
                putpixel(buffer, i, j, window->inner[c++], vbe_info->pitch);
            }
    }

     if(!HAS_FLAG(window->flags, GFX_HIDE_HEADER)){            
        /* Draw main frame of window with title bar and borders */
#ifdef __WINDOWS_95
        color_t header_color = window->in_focus ? 0x1 : COLOR_VGA_DARK_GRAY;
        color_t header_text_color = window->in_focus ? 0x0f : COLOR_VGA_LIGHT_GRAY;
#else 
        color_t header_color = theme->window.background;
        color_t header_text_color = window->in_focus ? 0x00 : COLOR_VGA_LIGHT_GRAY;
#endif // __WINDOWS_95

        vesa_fillrect(buffer, window->x+6, window->y-4, window->width-10, 12, header_color);

        /* top */
        vesa_line_horizontal(buffer, window->x+4, window->y-4, window->width-8, border_dark);
        vesa_line_horizontal(buffer, window->x+4, window->y-3, window->width-8, border_light);

        /* left */
        vesa_line_horizontal(buffer, window->x+4, window->y+8, window->width-8, border_dark);
        vesa_line_horizontal(buffer, window->x+4, window->y+7, window->width-8, background_color);

        if(window->in_focus){
#ifndef __WINDOWS_95
            for (int i = 0; i < 3; i++){
                vesa_line_horizontal(buffer, window->x+8, (window->y-1)+i*3, window->width-16, background_color);
                vesa_line_horizontal(buffer, window->x+8, (window->y-2)+i*3, window->width-16, border_light);
            }
#endif // __WINDOWS_95
        }

        /* Title */
        int title_position = (window->width/2) - ((strlen(window->name)*8)/2);
        vesa_fillrect(buffer, window->x+title_position, window->y-2, strlen(window->name)*8 + 4, 8, header_color);
        vesa_write_str(buffer, window->x+title_position+4, window->y-2, window->name, header_text_color);
    }

    if(!HAS_FLAG(window->flags, GFX_HIDE_BORDER) && !HAS_FLAG(window->flags, GFX_IS_HIDDEN)){
         /* bottom */
        vesa_fillrect(buffer, window->x+4, window->y+window->height-8, window->width-8, 4, theme->window.background);
        vesa_line_horizontal(buffer, window->x+4, window->y+window->height-4, window->width-8, border_dark);
        vesa_line_horizontal(buffer, window->x+4, window->y+window->height-5, window->width-8, background_color);

        vesa_line_horizontal(buffer, window->x+4, window->y+window->height-8, window->width-8, border_dark);
        vesa_line_horizontal(buffer, window->x+4, window->y+window->height-7, window->width-8, border_light);
        
        /* left */
        vesa_fillrect(buffer, window->x+4, window->y+8, 4, window->height-4-8, theme->window.background);
        vesa_line_vertical(buffer, window->x+4, window->y-3, window->height, border_dark);
        vesa_line_vertical(buffer, window->x+5, window->y-3, window->height-1, border_light);

        vesa_line_vertical(buffer, window->x+8, window->y+8, window->height-16, border_dark);
        vesa_line_vertical(buffer, window->x+7, window->y+8, window->height-16, background_color);

        /* right */
        vesa_fillrect(buffer, window->x+window->width-8, window->y+8, 4, window->height-4-8, theme->window.background);
        vesa_line_vertical(buffer, window->x+window->width-4, window->y-4, window->height, border_dark);
        vesa_line_vertical(buffer, window->x+window->width-5, window->y-4, window->height, background_color);

        vesa_line_vertical(buffer, window->x+window->width-8, window->y+8, window->height-16, border_dark);
        vesa_line_vertical(buffer, window->x+window->width-7, window->y+8, window->height-16, border_light);
    }

    if(!HAS_FLAG(window->flags, GFX_HIDE_HEADER) && !HAS_FLAG(window->flags, GFX_NO_OPTIONS)){

        /* full screen */
        if(window->is_resizable){    
            vesa_inner_box(buffer, window->x+window->width-46+6,  window->y-3, 10, 9, theme->window.background);
            vesa_write_str(buffer, window->x+window->width-44+6,  window->y-2, "^", COLOR_VGA_DARK_GRAY);
        }

        /* Minimize */
        vesa_inner_box(buffer, window->x+window->width-34+6,  window->y-3, 10, 9, theme->window.background);
        vesa_write_str(buffer, window->x+window->width-32+6,  window->y-2, "-", COLOR_VGA_DARK_GRAY);

        /* Exit */
        vesa_inner_box(buffer, window->x+window->width-22+6,  window->y-3, 10, 9, theme->window.background);
        vesa_write_str(buffer, window->x+window->width-20+6,  window->y-2, "X", COLOR_VGA_DARK_GRAY);
    }

    window->changed = 0;
    return;
}   

void gfx_window_set_resizable()
{
    $process->current->gfx_window->is_resizable = 1;
}

/* under construction */
static void gfx_window_resize(struct window* w, int width, int height)
{
    /* Allocate new inner buffer, copy over old buffer, free old buffer, update struct */
    uint8_t* new_buffer = kalloc(width*height);
    if(new_buffer == NULL){
        warningf("Failed to allocate new buffer for window resize\n");
        return;
    }

    uint8_t* old = w->inner;

    //memcpy(new_buffer, old, w->inner_height*w->inner_width > width*width ? width*width : w->inner_height*w->inner_width);

    /* problem: if resizing from a larger to smaller window, what happens to data that will be "offscreen" */
    w->inner_height = height;
    w->inner_width = width;
    w->width = width + 16;
    w->height = height + 16;
    w->pitch = width;

    /* Copy over */
    w->inner = new_buffer;

    struct gfx_event e = {
        .data = width,
        .data2 = height,
        .event = GFX_EVENT_RESOLUTION
    };
    gfx_push_event(w, &e);

    w->changed = 1;

    kfree(old);
}

/**
 * @brief Default handler for click event from mouse on given window.
 * 
 * @param window Window that was clicked.
 * @param x 
 * @param y 
 */
static void gfx_default_click(struct window* window, int x, int y)
{
    if(HAS_FLAG(window->flags, GFX_NO_OPTIONS)) return;
    
    if(gfx_point_in_rectangle(window->x, window->y, window->x+window->width, window->y+10, x, y)){
        //dbgprintf("Clicked %s header\n", window->name);
    }

    /* Exit button */
    if (x >= window->x + window->width - 22 + 6 && x <= window->x + window->width - 22 + 6 + 10 && y >= window->y - 3 && y <= window->y - 3 + 9) {
        dbgprintf("[GFX WINDOW] Clicked %s exit button\n", window->name);
        struct gfx_event e = {
            .data = 0,
            .data2 = 0,
            .event = GFX_EVENT_EXIT
        };
        gfx_push_event(window, &e);
        return; 
    }

    /* Minimize button */
    if (x >= window->x + window->width - 34 + 6 && x <= window->x + window->width - 34 + 6 + 10 && y >= window->y - 3 && y <= window->y - 3 + 9) {
        dbgprintf("[GFX WINDOW] Clicked %s minimize button\n", window->name);
        
        /* toggle IS_HIDDEN flag */
        if(HAS_FLAG(window->flags, GFX_IS_HIDDEN)){
            window->flags &= ~GFX_IS_HIDDEN;
        } else {
            window->flags |= GFX_IS_HIDDEN;
        }

        window->changed = true;
        return;
    }

    /* Full screen button */
    if (x >= window->x + window->width - 46 + 6 && x <= window->x + window->width - 46 + 6 + 10 && y >= window->y - 3 && y <= window->y - 3 + 9 && window->is_resizable) {
        dbgprintf("[GFX WINDOW] Clicked %s full screen button\n", window->name);
        if(window->is_maximized.state == 0){
            window->is_maximized.state = 1;
            window->is_maximized.width = window->width+4;
            window->is_maximized.height = window->height;

            window->ops->maximize(window);

        } else {
            window->is_maximized.state = 0;
            window->width = window->is_maximized.width;
            window->height = window->is_maximized.height;

            window->ops->resize(window, window->width-16, window->height-16);
        }

        struct gfx_event e = {
            .data = window->inner_width,
            .data2 = window->inner_height,
            .event = GFX_EVENT_RESOLUTION
        };
        gfx_push_event(window, &e);

        window->is_moving.state = GFX_WINDOW_STATIC;
        window->is_moving.x = x;
        window->is_moving.y = y;

        window->resize = 0;

        return;
    }

}


static void gfx_default_hover(struct window* window, int x, int y)
{
    if(window->is_moving.state == GFX_WINDOW_MOVING && !HAS_FLAG(window->flags, GFX_IS_IMMUATABLE)){

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

    if(window->resize && window->is_resizable){
        window->ops->resize(window, x-window->x-8, y-window->y-8);
    }
}

static void gfx_default_mouse_down(struct window* window, int x, int y)
{
    if(gfx_point_in_rectangle(window->x, window->y-2, window->x+window->width-60, window->y+10, x, y)){
        window->is_moving.state = GFX_WINDOW_MOVING;
        window->is_moving.x = x;
        window->is_moving.y = y;
    }

    if(gfx_point_in_rectangle(window->x+window->width-8,  window->y+window->height-8, window->x+window->width,  window->y+window->height, x, y) && window->is_resizable){
        dbgprintf("Clicked resize\n");
        window->resize = 1;
    }
}

static void gfx_default_mouse_up(struct window* window, int x, int y)
{
    if(gfx_point_in_rectangle(window->x, window->y-2, window->x+window->width, window->y+10, x, y)){
        window->is_moving.state = GFX_WINDOW_STATIC;
        window->is_moving.x = x;
        window->is_moving.y = y;

        window->changed = 1;
    }

    if(gfx_point_in_rectangle(window->x+window->width-8,  window->y+window->height-8, window->x+window->width,  window->y+window->height, x, y) && window->is_resizable){
        dbgprintf("Unclicked resize\n");
        window->resize = 0;
        window->changed = 1;
    }
}

int kernel_gfx_window_border_color(uint8_t color)
{
    if($process->current->gfx_window == NULL) return -1;
    $process->current->gfx_window->color.border = color;

    return ERROR_OK;
}

int gfx_destroy_window(struct window* w)
{
    ERR_ON_NULL(w);

    ENTER_CRITICAL();
    
    gfx_composition_remove_window(w);

    kfree(w->inner);
    w->owner->gfx_window = NULL;
    kfree(w);

    LEAVE_CRITICAL();

    return ERROR_OK;
}


static int gfx_window_maximize(struct window* window)
{
    ERR_ON_NULL(window);

    gfx_window_resize(window, vbe_info->width-16, vbe_info->height-40);

    window->ops->move(window, 0, 26);

    window->changed = 1;

    return ERROR_OK;
}

/**
 * @brief Creates a new window and attaches it to the process who called the function.
 * Allocates the inner buffer in the processes personal memory.
 * @param width 
 * @param height 
 * @return struct window* 
 */
struct window* gfx_new_window(int width, int height, window_flag_t flags)
{
    if($process->current->gfx_window != NULL)
        return $process->current->gfx_window;

    struct window* w = create(struct window);
    if(w == NULL){
        warningf("window is NULL\n");
        return NULL;
    }

    w->inner = kalloc(width*height);
    if(w->inner == NULL){
        warningf("Inner window is NULL\n");
        kfree(w);
        return NULL;
    }
    memset(w->inner, HAS_FLAG(flags, GFX_IS_TRANSPARENT) ? 255 : 0, width*height);
    memset(w->events.list, 0, sizeof(struct gfx_event)*GFX_MAX_EVENTS);

    w->flags = flags;

    /* Set default ops */
    w->ops = &default_window_ops;
    w->draw = &default_window_draw_ops;

    w->inner_height = height;
    w->inner_width = width;

    w->width = width;
    w->height = height;

    if(!HAS_FLAG(w->flags, GFX_HIDE_HEADER)){
        w->width += 16;
        w->height += 16;
    }

    w->owner = $process->current;
    w->pitch = w->inner_width;
    w->x = 32;
    w->y = 32;

    w->changed = 1;

    w->color.border = 0;
    w->color.header = 0;
    w->color.text = 0;
    w->spinlock = 0;

    w->next = NULL;

    if(flags & GFX_IS_RESIZABLE){
        w->is_resizable = 1;
    } else {
        w->is_resizable = 0;
    }
    w->resize = 0;
    
    w->events.head = 0;
    w->events.tail = 0;

    w->is_maximized.state = 0;
    w->is_maximized.width = 0;
    w->is_maximized.height = 0;

    w->is_moving.state = GFX_WINDOW_STATIC;
    /* Window can just use the name of the owner? */
    memcpy(w->name, $process->current->name, strlen($process->current->name)+1);
    memset(w->header, 0, GFX_MAX_WINDOW_NAME_SIZE);
    w->in_focus = 0;

    dbgprintf("[Window] Created new window for %s at 0x%x: inner 0x%x (total %x - %x)\n", $process->current->name, w, w->inner, sizeof(struct window), width*height);

    $process->current->gfx_window = w;
    gfx_composition_add_window(w);

    return w;
}