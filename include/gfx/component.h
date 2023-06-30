#ifndef __GFX_COMPONENT_H
#define __GFX_COMPONENT_H

#include <stdint.h>
#include <gfx/window.h>

enum gfx_components {
    GFX_BUTTON,
    GFX_RECTANGLE
};


struct gfx_component {
    struct gfx_component* next;

    uint8_t type;

    uint8_t color;
    uint16_t x, y;
    uint16_t width, height;

    void (*click)(struct window);
};

int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y);


#endif /* __GFX_COMPONENT_H */
