#include <gfx/component.h>
#include <vesa.h>

int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y)
{
    return (x >= x1 && x <= x2) && (y >= y1 && y <= y2);
}

void gfx_draw_component(uint8_t* buffer, struct gfx_component* c)
{
    switch (c->type)
    {
    case GFX_RECTANGLE:
        vesa_fillrect(buffer, c->x, c->y, c->x + c->width, c->y + c->height, c->color);
        break;
    
    default:
        break;
    }
}