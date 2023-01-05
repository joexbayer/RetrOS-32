#include <vesa.h>
#include <vbe.h>
#include <gfx/window.h>
#include <scheduler.h>
#include <util.h>
#include <memory.h>
#include <serial.h>
#include <rtc.h>
#include <timer.h>
#include <colors.h>
#include <screen.h>
#include <mouse.h>
#include <gfx/component.h>

static struct gfx_window* order;
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

void gfx_order_push_front(struct gfx_window* w)
{
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
}

void gfx_composition_add_window(struct gfx_window* w)
{
    if(order == NULL){
        order = w;
        return;
    }
    
    struct gfx_window* iter = order;
    order = w;
    order->next = iter;
}

void gfx_mouse_event(int x, int y, char flags)
{
    dbgprintf("[GFX MOUSE] Event registered.\n");
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

            return i->hover(i, x, y);       
        }
    
    /* No window was clicked. */
}

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

        /* Main composition loop */
        if(gfx_check_changes(order)){
            memset(gfx_composition_buffer, VESA8_COLOR_DARK_TURQUOISE, buffer_size);

            /* Draw windows in reversed order */
            gfx_recursive_draw(order);
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

        STI();

        sleep(2);

        /* Copy buffer over to framebuffer. */
        memcpy((uint8_t*)vbe_info->framebuffer, gfx_composition_buffer, buffer_size-1);

        if(mouse_ret){
            gfx_mouse_event(m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);


    }
}