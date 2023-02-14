#include <gfx/api.h>
#include <gfx/gfxlib.h>
#include <lib/graphics.h>

int gfx_syscall_hook(int option, void* data)
{
    switch (option)
    {
    case GFX_DRAW_CHAR:;
        struct gfx_char* c = (struct gfx_char*)data;
        gfx_draw_char(c->x, c->y, c->data, c->color);
        break;
    
    default:
        break;
    }

    return 0;
}