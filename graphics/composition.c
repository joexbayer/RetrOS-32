/**
 * @file composition.c
 * @author Joe Bayer (joexbayer)
 * @brief Window Server composition and window manage api.
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <vesa.h>
#include <vbe.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <scheduler.h>
#include <util.h>
#include <memory.h>
#include <serial.h>
#include <rtc.h>
#include <timer.h>
#include <colors.h>
#include <mouse.h>
#include <gfx/component.h>
#include <sync.h>

static struct gfx_window* order;
static mutex_t order_lock;

static uint8_t* gfx_composition_buffer;

/* Mouse globals */
static char gfx_mouse_state = 0;
static struct mouse m;

int gfx_check_changes(struct gfx_window* w)
{
    while(w != NULL){
        if(w->changed)
            return 1;
        w = w->next;
    }
    return 0;
}

void gfx_recursive_draw(struct gfx_window* w)
{
    if(w->next != NULL)
        gfx_recursive_draw(w->next);
    
    gfx_draw_window(gfx_composition_buffer, w);
}

/**
 * @brief Push a window to the front of "order" list.
 * Changing its z-axis position in the framebuffer.
 * @param w 
 */
void gfx_order_push_front(struct gfx_window* w)
{
    acquire(&order_lock);
    for (struct gfx_window* i = order; i != NULL; i = i->next)
    {
        if(i->next == w){
            i->next = w->next;
            break;
        }
    }
    
    order->in_focus = 0;
    struct gfx_window* save = order;
    order = w;
    w->next = save;
    order->in_focus = 1;

    w->changed = 1;

    release(&order_lock);
}

/**
 * @brief Removes a window from the "order" list.
 * Important keep the order list intact even if removing the first element.
 * @param w 
 */
void gfx_composition_remove_window(struct gfx_window* w)
{
    acquire(&order_lock);

    if(order == w)
    {
        order = w->next;
        if(order != NULL){
            order->changed = 1;
            order->in_focus = 1;
        }
        goto gfx_composition_remove_window_exit;
    }

    struct gfx_window* iter = order;
    while(iter != NULL && iter->next != w)
        iter = iter->next;
    if(iter == NULL){
        goto gfx_composition_remove_window_exit;
    }

    iter->next = w->next;
    
gfx_composition_remove_window_exit:

    dbgprintf("[GFX] Removing window\n");
    release(&order_lock);
}

/**
 * @brief Adds new window in the "order" list.
 * 
 * @param w 
 */
void gfx_composition_add_window(struct gfx_window* w)
{
    acquire(&order_lock);

    if(order == NULL){
        order = w;
        order->in_focus = 1;
        release(&order_lock);
        return;
    }
    
    struct gfx_window* iter = order;
    order->in_focus = 0;
    order = w;
    order->in_focus = 1;
    order->next = iter;

    release(&order_lock);
}

/**
 * @brief raw mouse event handler for window API.
 * 
 * @param x 
 * @param y 
 * @param flags 
 */
void gfx_mouse_event(int x, int y, char flags)
{
    for (struct gfx_window* i = order; i != NULL; i = i->next)
        if(gfx_point_in_rectangle(i->x, i->y, i->x+i->width, i->y+i->height, x, y)){
            /* on click when left mouse down */
            if(flags & 1 && gfx_mouse_state == 0){
                gfx_mouse_state = 1;
                i->mousedown(i, x, y);

                if(i != order)
                    gfx_order_push_front(i);

            } else if(!(flags & 1) && gfx_mouse_state == 1) {
                /* If mouse state is "down" send click event */
                gfx_mouse_state = 0;
                i->click(i, x, y);
                i->mouseup(i, x, y);
            }

            i->hover(i, x, y);
            return;
        }
    
    /* No window was clicked. */
}

/**
 * @brief Sample kthread to debug windows
 */
void gfx_window_debugger()
{
    gfx_new_window(300, 300);
    while (1)
    {
        int prog = 0;
        __gfx_draw_rectangle(0, 0, 300, 300, GFX_WINDOW_BG_COLOR);
        for (struct gfx_window* i = order; i != NULL; i = i->next)
        {
            __gfx_draw_format_text(5, 5+prog*64, VESA8_COLOR_BLACK, "%s", i->owner->name);
            __gfx_draw_format_text(5, 5+prog*64+8, VESA8_COLOR_BLACK, " - Inner: 0x%x (%d bytes)", i->inner, i->inner_height*i->inner_width);
            __gfx_draw_format_text(5, 5+prog*64+16, VESA8_COLOR_BLACK, " - Location: 0x%x", i);
            
            prog++;
        }
        sleep(1000);
        
    }
}

void gfx_init()
{
    mutex_init(&order_lock);
}

/**
 * @brief Main window server kthread entry function
 * Allocates a second framebuffer that will be memcpy'd to the VGA framebuffer.
 * 
 * Handles mouse events and pushes them to correct window.
 * Only redraws screen if a window has changed.
 * 
 * In current state its still very slow.
 */
void gfx_compositor_main()
{
    int buffer_size = vbe_info->width*vbe_info->height*(vbe_info->bpp/8)+1;

    dbgprintf("[WSERVER] %d bytes allocated for composition buffer.\n", buffer_size);
    gfx_composition_buffer = (uint8_t*) palloc(buffer_size);

    while(1)
    {
        
        /**
         * Problem with interrupts from mouse?
         * Manages to corrupt some register or variables.
         * 
         * Should also NOT redraw entire screen, only things that change.
         */
        CLI();
        int test = rdtsc();
        int mouse_ret = mouse_event_get(&m);
        int window_ret = gfx_check_changes(order);

        /* Main composition loop */
        if(window_ret){
            memset(gfx_composition_buffer, VESA8_COLOR_DARK_TURQUOISE, buffer_size);
            /* Draw windows in reversed order */
            //acquire(&order_lock);
            gfx_recursive_draw(order);  
            //release(&order_lock);
        }

        vesa_fillrect(gfx_composition_buffer, 0, 480-25, 640, 25, VESA8_COLOR_LIGHT_GRAY3);
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-25, 640, VESA8_COLOR_LIGHT_GRAY1); 
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-26, 640, VESA8_COLOR_LIGHT_GRAY1);

        vesa_inner_box(gfx_composition_buffer, 638-80, 480-22, 80, 19);
        vesa_put_icon32(gfx_composition_buffer, 10, 10);

        struct time time;
        get_current_time(&time);
        vesa_printf(gfx_composition_buffer, 638-65, 480-16, VESA8_COLOR_BLACK, "%d:%d %s", time.hour > 12 ? time.hour-12 : time.hour, time.minute, time.hour > 12 ? "PM" : "AM");

        vesa_printf(gfx_composition_buffer, 8, 480-16, VESA8_COLOR_DARK_BLUE, "%d", (rdtsc() - test)-100000);


        vesa_printf(gfx_composition_buffer, 100, 480-16, VESA8_COLOR_DARK_BLUE, "%d", (timer_get_tick()*10) % 1000);

        STI();

        sleep(2);

        if(mouse_ret || window_ret)
            memcpy((uint8_t*)vbe_info->framebuffer, gfx_composition_buffer, buffer_size-1);
        /* Copy buffer over to framebuffer. */

        if(mouse_ret){
            gfx_mouse_event(m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);


    }
}   