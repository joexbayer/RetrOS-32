#ifndef __WINDOW_H
#define __WINDOW_H

#include <stdint.h>
struct gfx_window;
#include <gfx/component.h>
#include <terminal.h>
#include <pcb.h>
#include <colors.h>

#define GFX_MAX_WINDOW_NAME_SIZE 20
#define GFX_WINDOW_BG_COLOR VESA8_COLOR_LIGHT_GRAY3
#define GFX_WINDOW_TITLE_HEIGHT 12

enum window_states {
    GFX_WINDOW_MOVING,
    GFX_WINDOW_STATIC
};

struct gfx_window {
    struct gfx_window* next;
    struct gfx_window* prev;

    char name[GFX_MAX_WINDOW_NAME_SIZE];
    
    uint16_t x, y;
    uint16_t width, height, inner_width, inner_height;

    /* TODO: Click function should also take a mouse event  */
    void (*click)(struct gfx_window*, int x, int y);
    void (*hover)(struct gfx_window*, int x, int y);
    void (*mousedown)(struct gfx_window*, int x, int y);
    void (*mouseup)(struct gfx_window*, int x, int y);

    /* Pointer to inner memory where applications draw. */
    uint8_t* inner;

    struct sticky {
        char state;
        uint16_t x;
        uint16_t y;
    } is_moving;

    struct pcb* owner;

    uint8_t in_focus;

    char changed;
};

void gfx_draw_window(uint8_t* buffer, struct gfx_window* window);
struct gfx_window* gfx_new_window(int width, int height);
int gfx_destory_window(struct gfx_window* w);

/* Default mouse event hooks */
void gfx_default_click(struct gfx_window* window, int x, int y);
void gfx_default_hover(struct gfx_window* window, int x, int y);
void gfx_default_mouse_down(struct gfx_window* window, int x, int y);
void gfx_default_mouse_up(struct gfx_window* window, int x, int y);


#endif //  __WINDOW_H   