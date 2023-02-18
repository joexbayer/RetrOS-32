#include <gfx/api.h>
#include <gfx/gfxlib.h>
#include <lib/graphics.h>

int gfx_syscall_hook(int option, void* data)
{
    switch (option)
    {
    case GFX_DRAW_CHAR_OPT:;
        struct gfx_char* c = (struct gfx_char*)data;
        __internal_gfx_draw_char(c->x, c->y, c->data, c->color);
        break;
    case GFX_DRAW_RECTANGLE_OPT:;
        struct gfx_rectangle* r = (struct gfx_rectangle*)data;
        __internal_gfx_draw_rectangle(r->x, r->y, r->width, r->height, r->color);
        break;
    default:
        break;
    }

    return 0;
}