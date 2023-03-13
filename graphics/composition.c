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
#include <gfx/events.h>
#include <keyboard.h>
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
#include <assert.h>

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
 * 
 * Assuming w is NOT already the first element.
 * @param w 
 */
void gfx_order_push_front(struct gfx_window* w)
{
    acquire(&order_lock);

    assert(w != order);

    /* remove w from order list */
    for (struct gfx_window* i = order; i != NULL; i = i->next)
    {
        if(i->next == w){
            i->next = w->next;
            break;
        }
    }
    
    /* Replace order with w, pushing old order back. */
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

                /* If clicked window is not in front, push it. */
                if(i != order){
                    gfx_order_push_front(i);
                }

                /* TODO: push mouse gfx event to window */

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

void gfx_init()
{
    mutex_init(&order_lock);
}

uint8_t edim1_to_vga(uint8_t color) {
    // Extract the red, green, and blue components of the color
    uint8_t r = (color >> 5) & 0x7;
    uint8_t g = (color >> 2) & 0x7;
    uint8_t b = color & 0x3;

    // Scale the color components to the range 0-63
    r = r * 63 / 7;
    g = g * 63 / 7;
    b = b * 63 / 3;

    // Combine the color components into a single byte
    return (r << 5) | (g << 2) | b;
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

    /* Main composition loop */
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
        int mouse_changed = mouse_event_get(&m);
        int window_changed = gfx_check_changes(order);
        
        
        char key = kb_get_char();
        if(key != -1){
            struct gfx_event e = {
                .data = key,
                .event = GFX_EVENT_KEYBOARD
            };
            gfx_push_event(order, &e);
        }

        if(window_changed){
            
            memset(gfx_composition_buffer, 41, buffer_size);
            for (int i = 0; i < 320; i++)
            {
                for (int j = 0; j < 240; j++)
                {
                    putpixel(gfx_composition_buffer, i, j, forman[j*320 + i], vbe_info->pitch);
                }
                
            }
            
            /* Draw windows in reversed order */
            //acquire(&order_lock);
            gfx_recursive_draw(order);
            //release(&order_lock);
        }

        vesa_fillrect(gfx_composition_buffer, 0, 480-25, 640, 25, COLOR_GRAY_DEFAULT);
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-25, 640, COLOR_GRAY_LIGHT); 
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-26, 640, COLOR_GRAY_LIGHT);

        vesa_inner_box(gfx_composition_buffer, 638-80, 480-22, 80, 19);
        vesa_put_icon32(gfx_composition_buffer, 10, 10);

        struct time time;
        get_current_time(&time);
        vesa_printf(gfx_composition_buffer, 638-65, 480-16, COLOR_BLACK, "%d:%d %s", time.hour > 12 ? time.hour-12 : time.hour, time.minute, time.hour > 12 ? "PM" : "AM");

        vesa_printf(gfx_composition_buffer, 8, 480-16, COLOR_DARK_BLUE, "%d", (rdtsc() - test)-100000);


        vesa_printf(gfx_composition_buffer, 100, 480-16, COLOR_DARK_BLUE, "%d", (timer_get_tick()*10) % 1000);

        STI();

        sleep(2);

        if(mouse_changed || window_changed)
            memcpy((uint8_t*)vbe_info->framebuffer, gfx_composition_buffer, buffer_size-1);
        /* Copy buffer over to framebuffer. */

        if(mouse_changed){
            gfx_mouse_event(m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);


    }
}   