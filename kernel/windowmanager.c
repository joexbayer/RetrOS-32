#include <windowmanager.h>
#include <screen.h>
#include <serial.h>
#include <pcb.h>

#define USABLE_WIDTH (SCREEN_WIDTH-1)
#define USABLE_HEIGHT (SCREEN_HEIGHT-1)

void draw_window(struct window* w)
{
    if(w->visable == 0)
        return;

    //dbgprintf("[WM] Window: Anchor %d, width: %d, height: %d\n", w->x, w->width, w->height);

    for (uint8_t i = 0; i < w->width; i++)
    {
        scrput(w->x+i, w->y, 205, w->color);
        scrput(w->x+i, w->y+w->height, 196, w->color);
    }

    for (uint8_t i = 0; i < w->height; i++)
    {
        scrput(w->x, w->y+i, 179, w->color);
        scrput(w->x+w->width, w->y+i, 179, w->color);
    }

    scrput(w->x, w->y, 213, w->color);
    scrput(w->x+w->width, w->y, 184, w->color);

    scrput(w->x, w->y+w->height, 192, w->color);
    scrput(w->x+w->width, w->y+w->height, 217, w->color);
    scrprintf(w->x+2, w->y, w->name);
}

int attach_window(struct window* w)
{
    current_running->window = w;
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

uint8_t is_window_visable()
{
    if(current_running->window != NULL)
        return current_running->window->visable;
    
    return 0;
        
}

struct terminal_state* get_terminal_state()
{
    return &current_running->window->state;
}