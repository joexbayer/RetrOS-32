#include <windowmanager.h>
#include <screen.h>

void draw_window(struct window* w)
{
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