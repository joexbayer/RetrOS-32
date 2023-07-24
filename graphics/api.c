#include <gfx/api.h>
#include <gfx/gfxlib.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <assert.h>
#include <errors.h>

int gfx_syscall_hook(int option, void* data, int flags)
{
    if(option >= GFX_DRAW_UNUSED || option < 0) return -1;
    if(!(flags >= 0 && flags < 255))return -1;

    ERR_ON_NULL(data);
    ERR_ON_NULL(current_running);

    switch (option)
    {
    case GFX_DRAW_CHAR_OPT:;
        struct gfx_char* c = (struct gfx_char*)data;
        kernel_gfx_draw_char(current_running->gfx_window, c->x, c->y, c->data, c->color);
        break;
    case GFX_DRAW_RECTANGLE_OPT:;
        struct gfx_rectangle* r = (struct gfx_rectangle*)data;
        kernel_gfx_draw_rectangle(current_running->gfx_window, r->x, r->y, r->width, r->height, r->color);
        break;
    
    case GFX_DRAW_CIRCLE_OPT:;
        struct gfx_circle* circle = (struct gfx_circle*)data;
        kernel_gfx_draw_circle(current_running->gfx_window, circle->x, circle->y, circle->r, circle->color, circle->fill);
        break;
    
    case GFX_DRAW_LINE_OPT:;
        struct gfx_line* line = (struct gfx_line*)data;
        kernel_gfx_draw_line(current_running->gfx_window, line->x0, line->y0, line->x1, line->y1, line->color);
        break;
    
    case GFX_DRAW_PIXEL:;
        struct gfx_pixel* pixel = (struct gfx_pixel*)data;
        kernel_gfx_draw_pixel(current_running->gfx_window, pixel->x, pixel->y, pixel->color);
        break;

    case GFX_EVEN_LOOP_OPT:
        return gfx_event_loop((struct gfx_event*)data, flags);

    default:
        break;
    }

    gfx_commit();

    return ERROR_OK;
}