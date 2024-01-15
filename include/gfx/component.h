#ifndef __GFX_COMPONENT_H
#define __GFX_COMPONENT_H

#include <stdint.h>
#include <kutils.h>
#include <gfx/window.h>
#include <gfx/events.h>

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

struct gfx_input {
    uint16_t x, y;
    uint16_t width, height;
    char placeholder[32];
    int clicked;
    byte_t buffer[256];
    uint8_t buffer_size;
};
int gfx_input_putchar(struct gfx_input* input, char c);

struct gfx_input_manager {
    struct gfx_input inputs[32];
    uint8_t input_count;
};
int gfx_input_draw(struct window* w, struct gfx_input_manager* input);
int gfx_input_manager_add(struct gfx_input_manager* man, struct gfx_input input);
int gfx_input_event(struct gfx_input_manager* input, struct gfx_event* event);



int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y);


#endif /* __GFX_COMPONENT_H */
