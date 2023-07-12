#include <windowmanager.h>
#include <util.h>
#include <errors.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <gfx/events.h>
#include <sync.h>
#include <assert.h>
#include <memory.h>
#include <mouse.h>
#include <math.h>

/* default static prototype functions for windowmanager ops */

static int wm_default_add(struct windowmanager* wm, struct window* window);
static int wm_default_remove(struct windowmanager* wm, struct window* window);
static int wm_default_draw(struct windowmanager* wm, struct window* window);
static int wm_default_push_front(struct windowmanager* wm, struct window* window);
static int wm_default_mouse_event(struct windowmanager* wm, int x, int y, char flags);

/* default windowmanager ops struct */
static struct windowmanager_ops wm_default_wm_ops = {
    .add = wm_default_add,
    .remove = wm_default_remove,
    .draw = wm_default_draw,
    .push_front = wm_default_push_front,
    .mouse_event = wm_default_mouse_event
};

/* init new window manager with default ops and rest 0 */
int init_windowmanager(struct windowmanager* wm, int flags)
{
    ERR_ON_NULL(wm);
    WM_VALIDATE_FLAGS(wm);

    wm->spinlock = SPINLOCK_UNLOCKED;
    wm->composition_buffer_size = 0;
    wm->composition_buffer = NULL;
    wm->ops = &wm_default_wm_ops;
    wm->windows = NULL;
    wm->window_count = 0;

    /* flags */
    wm->flags = flags;
    
    /* state */
    wm->state = WM_INITIALIZED;

    WM_VALIDATE(wm);

    return ERROR_OK;
}


/* default static functions for windowmanager ops */

/**
 * @brief wm_default_add adds a window to the windowmanager
 * adds the window to the end of the list
 * @param wm  windowmanager
 * @param window  window to add
 * @return int  0 on success, error code otherwise
 */
static int wm_default_add(struct windowmanager* wm, struct window* window)
{
    ERR_ON_NULL(wm);
    
    if (wm->window_count == 0) {
        wm->windows = window;
        wm->window_count++;
        return ERROR_OK;
    }

    struct window* current = wm->windows;
    wm->windows = window;
    window->next = current;

    wm->window_count++;

    return ERROR_OK;
}

/**
 * @brief wm_default_remove removes a window from the windowmanager
 * 
 * @param wm windowmanager
 * @param window window to remove
 * @return int 0 on success, error code otherwise
 */
static int wm_default_remove(struct windowmanager* wm, struct window* window)
{
    ERR_ON_NULL(wm);

    if (wm->window_count == 0) {
        return -ERROR_WINDOW_NOT_FOUND;
    }

    assert(wm->windows != NULL);

    spin_lock(&wm->spinlock);

    if (wm->windows == window) {
        wm->windows = window->next;
        dbgprintf("Removing front window %s\n", window->name);
    } else {
        /* Find the window before the one we want to remove. */
        struct window* i;
        for (i = wm->windows; i != NULL && i->next != window; i = i->next);
        if (i){
            dbgprintf("Removing window %s\n", window->name);
            i->next = window->next;
        }
    }

    if(wm->windows) {
        wm->windows->changed = 1;
        wm->windows->in_focus = 1;
    }
    wm->window_count--;

    spin_unlock(&wm->spinlock);

    return ERROR_OK; 
}

/**
 * @brief wm_default_push_front pushes a window to the front of the windowmanager
 * Used to bring a window to the front of the screen
 * @param wm windowmanager
 * @param window window to push 
 * @return int 0 on success, error code otherwise 
 */
static int wm_default_push_front(struct windowmanager* wm, struct window* window)
{
    ERR_ON_NULL(wm);

    if(window == wm->windows){
        return ERROR_OK;
    }

    dbgprintf("Pushing window %s to front\n", window->name);

    PANIC_ON_ERR(wm->ops->remove(wm, window));

    spin_lock(&wm->spinlock);
    /* Replace wm->windows with window, pushing original wm->windows back. */
    wm->windows->in_focus = 0;
    window->next = wm->windows;
    wm->windows = window;
    wm->windows->in_focus = 1;

    window->changed = 1;
    wm->window_count++;

    spin_unlock(&wm->spinlock);

    return ERROR_OK;

}

/**
 * @brief wm_default_draw draws the windows from back to front recursively
 * TODO: add a max depth to this to prevent stack overflow
 * @warning this can cause a stack overflow if there are too many windows
 * @param wm windowmanager
 * @param window window to recursively draw from 
 * @return int 0 on success, error code otherwise
 */
static int wm_default_draw(struct windowmanager* wm, struct window* window)
{
    ERR_ON_NULL(wm);
    ERR_ON_NULL(window);

    if (wm->window_count == 0 || window == NULL) {
        return ERROR_OK;
    }

    if(window->next != NULL)
        wm->ops->draw(wm, window->next);
    
    gfx_draw_window(wm->composition_buffer, window);

    return ERROR_OK;
}

static int wm_default_mouse_event(struct windowmanager* wm, int x, int y, char flags)
{
    ERR_ON_NULL(wm);

    if (wm->window_count == 0) {
        return ERROR_OK;
    }
    /* NOTE: no spinlock needed as windows are not changed */

    /* iterate over windows and check if a window was clicked, from front to back. */
    for (struct window* i = wm->windows; i != NULL; i = i->next){
        /* if mouse is in window */
        if(gfx_point_in_rectangle(i->x, i->y, i->x+i->width, i->y+i->height, x, y)){
            /* on click when left mouse down */
            if((flags & MOUSE_LEFT) && wm->mouse_state == 0){
                wm->mouse_state = 1;
                i->ops->mousedown(i, x, y);
                
                /* If clicked window is not in front, push it. */
                if(i != wm->windows){
                    wm->ops->push_front(wm, i);
                }

            } else if(!(flags & MOUSE_LEFT) && wm->mouse_state == 1) {
                /* If mouse state is "down" send click event */
                wm->mouse_state = 0;
                i->ops->click(i, x, y);
                i->ops->mouseup(i, x, y);

                int offset = HAS_FLAG(i->flags, GFX_HIDE_HEADER) ? 0 : 8;

                uint16_t x2 = CLAMP( (x - (i->x+offset)), 0,  i->inner_width);
                uint16_t y2 = CLAMP( (y - (i->y+offset)), 0,  i->inner_height);

                /* Send mouse event */
                struct gfx_event e = {
                    .data = x2,
                    .data2 = y2,
                    .event = GFX_EVENT_MOUSE
                };
                gfx_push_event(i, &e);
            }

            /* always send a hover event */
            i->ops->hover(i, x, y);

            return ERROR_OK;
        }
    }

    return ERROR_OK;
}
