#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <colors.h>

struct window {

    char name[40];
    uint8_t anchor;
    uint8_t height;
    uint8_t width;
    uint8_t color;

};

void draw_window(struct window* w);
int get_window_width();
int get_window_height();

#endif /* __WINDOWMANAGER_H */
