#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <colors.h>

struct terminal_state {
    uint8_t column;
    uint8_t row;
    uint8_t color;
};

struct window {

    char name[40];
    uint8_t x;
    uint8_t y;
    uint8_t height;
    uint8_t width;
    uint8_t color;
    uint8_t visable;

    struct terminal_state state;

};

void draw_window(struct window* w);
int get_window_width();
int get_window_height();
int attach_window(struct window* w);
uint8_t is_window_visable();
void init_wm();

struct terminal_state* get_terminal_state();
#endif /* __WINDOWMANAGER_H */
