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

static struct gfx_window* order;
static uint8_t* gfx_composition_buffer;

void gfx_recursive_draw(struct gfx_window* w)
{
    if(w->next != NULL)
        gfx_recursive_draw(w->next);
    
    gfx_draw_window(gfx_composition_buffer, w);
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

void gfx_compositor_main()
{
    int buffer_size = vbe_info->width*vbe_info->height*(vbe_info->bpp/8)+1;

    dbgprintf("[WSERVER] %d bytes allocated for composition buffer.\n", buffer_size);
    gfx_composition_buffer = (uint8_t*) alloc(buffer_size);

    while(1)
    {
        CLI();
        /* Main composition loop */
        vesa_fill(gfx_composition_buffer, VESA8_COLOR_DARK_TURQUOISE); /* Background */

        /* Draw windows in reversed order */
        gfx_recursive_draw(order);

        vesa_fillrect(gfx_composition_buffer, 0, 480-25, 640, 25, VESA8_COLOR_LIGHT_GRAY3); /* Taskbar */
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-25, 640, VESA8_COLOR_LIGHT_GRAY1); /* contour taskbar */
        vesa_line_horizontal(gfx_composition_buffer, 0, 480-26, 640, VESA8_COLOR_LIGHT_GRAY1);

        vesa_inner_box(gfx_composition_buffer, 638-80, 480-22, 80, 19);
        vesa_put_icon32(gfx_composition_buffer, 10, 10);


        struct time time;
        get_current_time(&time);
        time.hour -= 1;
        vesa_printf(gfx_composition_buffer, 638-65, 480-16, VESA8_COLOR_BLACK, "%d:%d %s", time.hour > 12 ? time.hour-12 : time.hour, time.minute, time.hour > 12 ? "PM" : "AM");

        struct mouse m;
        mouse_get(&m);

        vesa_put_icon16(gfx_composition_buffer, m.x, m.y);

        /* Copy buffer over to framebuffer. */
        memcpy((uint8_t*)vbe_info->framebuffer, gfx_composition_buffer, buffer_size-1);

        CLI();

        yield();

    }
}