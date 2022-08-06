#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <colors.h>

struct window {

    uint8_t anchor;
    uint8_t height;
    uint8_t width;
    uint8_t color;

};

void draw_window(struct window* w);

#endif /* __WINDOWMANAGER_H */
