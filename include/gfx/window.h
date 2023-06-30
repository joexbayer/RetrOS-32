#ifndef __WINDOW_H
#define __WINDOW_H

#include <stdint.h>
#include <gfx/events.h>
struct window;
#include <gfx/component.h>
#include <terminal.h>
#include <pcb.h>
#include <colors.h>
#include <sync.h>

#define GFX_MAX_WINDOW_NAME_SIZE 20
#define GFX_WINDOW_BG_COLOR COLOR_BOX_GRAY_DEFAULT
#define GFX_WINDOW_TITLE_HEIGHT 12
#define GFX_MAX_EVENTS 10

enum window_states {
    GFX_WINDOW_MOVING,
    GFX_WINDOW_STATIC
};

typedef enum window_flags {
    GFX_IS_RESIZABLE = 1 << 0
} window_flag_t;

struct window {
    struct window* next;
    //struct window* prev;

    char name[GFX_MAX_WINDOW_NAME_SIZE];
    char header[GFX_MAX_WINDOW_NAME_SIZE];

    /* TODO: create proper buttons */
    char buttons[5][GFX_MAX_WINDOW_NAME_SIZE];
    
    uint16_t x, y;
    uint16_t width, height, inner_width, inner_height;
    uint16_t pitch;

    /* TODO: Click function should also take a mouse event  */
    void (*click)(struct window*, int x, int y);
    void (*hover)(struct window*, int x, int y);
    void (*mousedown)(struct window*, int x, int y);
    void (*mouseup)(struct window*, int x, int y);

    struct {
        struct gfx_event list[GFX_MAX_EVENTS];
        uint8_t head;
        uint8_t tail;
    } events;

    struct {
        uint8_t border;
        uint8_t header;
        uint8_t text;
    } color;

    /* Pointer to inner memory where applications draw. */
    uint8_t* inner;
    uint8_t is_resizable;
    uint8_t resize;

    struct sticky {
        char state;
        uint16_t x;
        uint16_t y;
    } is_moving;     

    struct pcb* owner;
    spinlock_t spinlock;

    uint8_t in_focus;

    char changed;
};

void gfx_draw_window(uint8_t* buffer, struct window* window);
struct window* gfx_new_window(int width, int height, window_flag_t flags);
int gfx_destory_window(struct window* w);
void gfx_window_set_resizable();

/* Default mouse event hooks */
void gfx_default_click(struct window* window, int x, int y);
void gfx_default_hover(struct window* window, int x, int y);
void gfx_default_mouse_down(struct window* window, int x, int y);
void gfx_default_mouse_up(struct window* window, int x, int y);

void gfx_window_resize(struct window* w, int width, int height);


#endif //  __WINDOW_H   