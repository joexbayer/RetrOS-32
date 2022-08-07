#include <windowmanager.h>
#include <screen.h>
#include <serial.h>
#include <pcb.h>

#define USABLE_WIDTH (SCREEN_WIDTH-1)
#define USABLE_HEIGHT (SCREEN_HEIGHT-1)

void draw_window(struct window* w)
{
    dbgprintf("[WM] Window: Anchor %d, width: %d, height: %d\n", w->anchor, w->width, w->height);

    for (uint8_t i = 0; i < w->width; i++)
    {
        scrput(w->anchor+i, w->anchor, 205, w->color);
        scrput(w->anchor+i, w->anchor+w->height, 196, w->color);
    }

    for (uint8_t i = 0; i < w->height; i++)
    {
        scrput(w->anchor, w->anchor+i, 179, w->color);
        scrput(w->anchor+w->width, w->anchor+i, 179, w->color);
    }

    scrput(w->anchor, w->anchor, 213, w->color);
    scrput(w->anchor+w->width, w->anchor, 184, w->color);

    scrput(w->anchor, w->anchor+w->height, 192, w->color);
    scrput(w->anchor+w->width, w->anchor+w->height, 217, w->color);
    scrprintf(w->anchor+2, w->anchor, "TERMINAL");
}

int get_window_height()
{
    if(current_running->window != NULL)
        return current_running->window->height;

    return SCREEN_HEIGHT;
}

int get_window_width()
{
    if(current_running->window != NULL)
        return current_running->window->width;

    return SCREEN_WIDTH;
}