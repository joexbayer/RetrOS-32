/**
 * @file windowmanager.c
 * @author Joe Bayer (joexbayer)
 * @brief Window manager implementation
 * @version 0.1
 * @date 2023-11-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <windowmanager.h>
#include <libc.h>
#include <errors.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <gfx/events.h>
#include <sync.h>
#include <assert.h>
#include <memory.h>
#include <mouse.h>
#include <math.h>
#include <vbe.h>

/* default static prototype functions for windowmanager ops */

static int wm_default_add(struct windowmanager* wm, struct window* window);
static int wm_default_remove(struct windowmanager* wm, struct window* window);
static int wm_default_draw(struct windowmanager* wm, struct window* window);
static int wm_default_push_front(struct windowmanager* wm, struct window* window);
static int wm_default_push_back(struct windowmanager* wm, struct window* window);
static int wm_default_mouse_event(struct windowmanager* wm, int x, int y, char flags);
static int wm_default_workspace(struct windowmanager* wm, int workspace);
static int wm_default_changes(struct windowmanager* wm);

/* default windowmanager ops struct */
static struct windowmanager_ops wm_default_wm_ops = {
    .add = wm_default_add,
    .remove = wm_default_remove,
    .draw = wm_default_draw,
    .push_front = wm_default_push_front,
    .push_back = wm_default_push_back,
    .mouse_event = wm_default_mouse_event,
    .workspace = wm_default_workspace,
    .init = init_windowmanager,
    .destroy = wm_destroy,
    .changes = wm_default_changes
};

/* init new window manager with default ops and rest 0 */
int init_windowmanager(struct windowmanager* wm, int flags)
{
    ERR_ON_NULL(wm);

    wm->composition_buffer_size = VBE_SIZE();
    wm->composition_buffer = kalloc(VBE_SIZE());
    if(wm->composition_buffer == NULL){
        dbgprintf("Failed to allocate composition buffer\n");
        return -ERROR_ALLOC;
    }
    memset(wm->composition_buffer, 0x0, VBE_SIZE());


    wm->spinlock = SPINLOCK_UNLOCKED;
    wm->windows = NULL;
    wm->window_count = 0;
    wm->mouse_state = 0;

    /* workspaces */
    wm->workspace = 0;
    for(int i = 0; i < WM_MAX_WORKSPACES; i++){
        wm->workspaces[i] = NULL;
    }

    /* flags */
    wm->flags = flags;
    
    /* state */
    wm->state = WM_INITIALIZED;

    dbgprintf("Initialized window manager\n");

    return ERROR_OK;
}

static int wm_default_changes(struct windowmanager* wm)
{
    ERR_ON_NULL(wm);
    WM_VALIDATE(wm);

    struct window* w = wm->windows;

    while(w != NULL){
        if(w->changed)
            return 1;
        w = w->next;
    }

    return 0;
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
    ERR_ON_NULL(window);

    if (wm->window_count == 0) {
        wm->windows = window;
        
        window->in_focus = 1;

        wm->window_count++;
        return ERROR_OK;
    }

    struct window* current = wm->windows;
    wm->windows = window;
    window->next = current;

    wm->window_count++;

    if(!HAS_FLAG(window->flags, GFX_HIDE_HEADER)){
        window->in_focus = 1;
        current->in_focus = 0;
    }


    return ERROR_OK;
}

static int wm_default_workspace(struct windowmanager* wm, int workspace)
{
    ERR_ON_NULL(wm);

    dbgprintf("Switching to workspace %d\n", workspace);

    if(workspace < 0 || workspace >= WM_MAX_WORKSPACES){
        return -ERROR_INVALID_ARGUMENTS;
    }

    /* store current windows in workspace */
    wm->workspaces[wm->workspace] = wm->windows;

    /* set new workspace */
    wm->windows = wm->workspaces[workspace];
    wm->workspace = workspace;

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

static int wm_default_push_back(struct windowmanager* wm, struct window* window)
{
    ERR_ON_NULL(wm);

    if(wm->window_count < 2){
        return ERROR_OK;
    }

    dbgprintf("Pushing window %s to back\n", window->name);

    window->in_focus = 0;

    PANIC_ON_ERR(wm->ops->remove(wm, window));

    spin_lock(&wm->spinlock);
    /* Replace wm->windows with window, pushing original wm->windows back. */
    struct window* i;
    for (i = wm->windows; i != NULL && i->next != NULL; i = i->next);
    if (i){
        i->next = window;
    }

    window->next = NULL;
    window->changed = 1;
    wm->window_count++;

    if(wm->windows){
        wm->windows->changed = 1;
        wm->windows->in_focus = 1;
    }

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

    if(!HAS_FLAG(window->flags, GFX_HIDE_HEADER)){
        wm->windows->in_focus = 0;
    }
    window->next = wm->windows;
    wm->windows = window;

    if(!HAS_FLAG(window->flags, GFX_HIDE_HEADER)){
        wm->windows->in_focus = 1;
    }

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
    if(window->next != NULL){
        wm->ops->draw(wm, window->next);
    }
    
    window->draw->draw(wm->composition_buffer, window);

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
            /* get coordinates inside of the window */
            int offset = HAS_FLAG(i->flags, GFX_HIDE_HEADER) ? 0 : 8;
            uint16_t x2 = CLAMP( (x - (i->x+offset)), 0,  i->inner_width);
            uint16_t y2 = CLAMP( (y - (i->y+offset)), 0,  i->inner_height);
            
            /* check if the pixel is transparent (255) */
            if(WINDOW_GET_PIXEL(i, x2, y2) == 255 && HAS_FLAG(i->flags, GFX_IS_TRANSPARENT)){
                continue;
            }

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

                /* Send mouse event to all windows that are not hidden */
                if(!HAS_FLAG(i->flags, GFX_IS_HIDDEN)){
                     /* Send mouse event */
                    struct gfx_event e = {
                        .data = x2,
                        .data2 = y2,
                        .event = GFX_EVENT_MOUSE
                    };
                    gfx_push_event(i, &e);
                }
            }

            /* always send a hover event */
            i->ops->hover(i, x, y);

            return ERROR_OK;
        }
        /**
         * @brief Special edge case:
         * When a window is moving (only 1 window should be moving at a time)
         * and the mouse is not in the window, the window should snap to the mouse.
         */
        if(i->is_moving.state == GFX_WINDOW_MOVING){
            i->ops->hover(i, x, y);
            return ERROR_OK;
        }
    }

    return ERROR_OK;
}

/**
 * @brief wm_init initializes a windowmanager
 * 
 * @param wm windowmanager
 * @return int 0 on success, error code otherwise
 */
struct windowmanager* wm_new(int flags)
{
    struct windowmanager* wm = create(struct windowmanager);
    if(wm == NULL){
        return NULL;
    }
    /* init krefs */
    kref_init(&wm->_krefs);
    
    wm->ops = &wm_default_wm_ops;

    /* init windowmanager */
    if(wm->ops->init(wm, flags) < 0){
        kfree(wm);
        return NULL;
    }

    kref_get(&wm->_krefs);
    return wm;
}

/**
 * @brief wm_destroy destroys a windowmanager
 * 
 * @param wm windowmanager
 * @return int 0 on success, error code otherwise
 */
int wm_destroy(struct windowmanager* wm)
{
    ERR_ON_NULL(wm);
    WM_VALIDATE(wm);

    /* check refs */
    if(kref_put(&wm->_krefs) > 0){
        return ERROR_OK;
    }

    /* free composition buffer */
    if(wm->composition_buffer != NULL){
        kfree(wm->composition_buffer);
    }

    /* free workspaces */
    // TODO

    /* free windows */

    /* free windowmanager */
    kfree(wm);

    return ERROR_OK;
}