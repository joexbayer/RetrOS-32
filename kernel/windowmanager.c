#include <windowmanager.h>
#include <util.h>
#include <errors.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <gfx/events.h>
#include <sync.h>

/* default static prototype functions for windowmanager ops */
static int default_add(struct windowmanager* wm, struct window* window);
static int default_remove(struct windowmanager* wm, struct window* window);
static int default_draw(struct windowmanager* wm, struct window* window);
static int default_push_front(struct windowmanager* wm, struct window* window);
static int default_mouse_event(struct windowmanager* wm, int x, int y, char type);

/* default windowmanager ops struct */
static struct windowmanager_ops default_wm_ops = {
    .add = default_add,
    .remove = default_remove,
    .draw = default_draw,
    .push_front = default_push_front,
    .mouse_event = default_mouse_event
};

/* default windowmanager struct */
static struct windowmanager default_wm = {
    .ops = &default_wm_ops,
    .composition_buffer_size = 0,
    .composition_buffer = NULL,
    .windows = NULL,
    .window_count = 0,
    .spinlock = SPINLOCK_UNLOCKED
};

/* default static functions for windowmanager ops */
static int default_add(struct windowmanager* wm, struct window* window)
{
    if (wm->window_count == 0) {
        wm->windows = window;
        wm->window_count++;
        return 0;
    }

    struct window* current = wm->windows;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = window;
    wm->window_count++;

    return 0;
}

static int default_remove(struct windowmanager* wm, struct window* window)
{
    if (wm->window_count == 0) {
        return -ERROR_WINDOW_NOT_FOUND;
    }

    assert(wm->windows != NULL);

    spin_lock(&wm->spinlock);

    if (wm->windows == window) {
        wm->windows = window->next;
    } else {
        /* Find the window before the one we want to remove. */
        struct window* i;
        for (i = wm->windows; i && i->next != window; i = i->next);
        if (i) i->next = window->next;
    }

    if(wm->windows) {
        wm->windows->changed = 1;
        wm->windows->in_focus = 1;
    }
    wm->window_count--;

    spin_unlock(&wm->spinlock);

    return 0;
}

static int push_front(struct windowmanager* wm, struct window* window)
{
    if(window == wm->windows){
        return 0;
    }

    RETURN_ON_ERR(wm->ops->remove(wm, window));

    spin_lock(&wm->spinlock);
    /* Replace wm->windows with window, pushing original wm->windows back. */
    wm->windows->in_focus = 0;
    window->next = wm->windows;
    wm->windows = window;
    wm->windows->in_focus = 1;

    window->changed = 1;

    spin_unlock(&wm->spinlock);

    return 0;

}

/* default draw should tail recursivly draw windows */
static int default_draw(struct windowmanager* wm, struct window* window)
{
    if (wm->window_count == 0) {
        return 0;
    }

    if(window == NULL){
        window = wm->windows;
    }

    if(window->next != NULL)
        wm->ops->draw(wm, window->next);
    
    gfx_draw_window(wm->composition_buffer, window);

    return 0;
}

static int default_mouse_event(struct windowmanager* wm, int x, int y, char type)
{
    if (wm->window_count == 0) {
        return 0;
    }

    for (struct window* i = wm->windows; i != NULL; i = i->next)
        if(gfx_point_in_rectangle(i->x, i->y, i->x+i->width, i->y+i->height, x, y)){
            /* on click when left mouse down */
            if((type & 1) && wm->mouse_state == 0){
                wm->mouse_state = 1;
                i->mousedown(i, x, y);

                /* If clicked window is not in front, push it. */
                if(i != wm->windows){
                    wm->ops->push_front(wm, i);
                }
            } else if(!(type & 1) && wm->mouse_state == 1) {
                /* If mouse state is "down" send click event */
                wm->mouse_state = 0;
                i->click(i, x, y);
                i->mouseup(i, x, y);

                uint16_t x2 = CLAMP( (x - (i->x+8)), 0,  i->inner_width);
                uint16_t y2 = CLAMP( (y - (i->y+8)), 0,  i->inner_height);

                struct gfx_event e = {
                    .data = x2,
                    .data2 = y2,
                    .event = GFX_EVENT_MOUSE
                };
                gfx_push_event(i, &e);
            }

            i->hover(i, x, y);
            return 0;
        }

    return 0;
}
