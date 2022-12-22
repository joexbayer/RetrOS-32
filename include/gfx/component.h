#ifndef __GFX_COMPONENT_H
#define __GFX_COMPONENT_H

#include <stdint.h>

enum gfx_components {
    GFX_BUTTON
};


struct gfx_component {
    uint8_t type;

    uint16_t x, y;
    uint16_t width, height;

    void (*click)(struct gfx_window);
};

int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y);


#endif /* __GFX_COMPONENT_H */
