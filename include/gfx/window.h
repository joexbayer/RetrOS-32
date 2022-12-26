#ifndef __WINDOW_H
#define __WINDOW_H

#include <stdint.h>
#include <colors.h>

#define GFX_MAX_WINDOW_NAME_SIZE 20
#define GFX_WINDOW_BG_COLOR VESA8_COLOR_LIGHT_GRAY3
#define GFX_WINDOW_TITLE_HEIGHT 12

enum window_states {
    MOVING,
    STATIC
};

struct gfx_window {
    struct gfx_window* next;

    char name[GFX_MAX_WINDOW_NAME_SIZE];
    
    uint16_t x, y;
    uint16_t width, height;

    /* TODO: Click function should also take a mouse event  */
    void (*click)(struct gfx_window*, int x, int y, char flags);

    /* Pointer to inner memory where applications draw. */
    uint8_t* inner;

    struct sticky {
        char state;
        uint16_t x;
        uint16_t y;
    } is_moving;

    uint8_t in_focus;
};

void gfx_draw_window(uint8_t* buffer, struct gfx_window* window);
void gfx_default_click(struct gfx_window* window, int x, int y, char flags);


#endif //  __WINDOW_H   