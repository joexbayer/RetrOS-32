#include <gfx/windowserver.h>
#include <keyboard.h>
#include <gfx/gfxlib.h>
#include <scheduler.h>
#include <gfx/events.h>
#include <memory.h>
#include <vbe.h>

static int ws_init(struct windowserver* ws);
static int ws_add(struct windowserver* ws, struct window* window);
static int ws_remove(struct windowserver* ws, struct window* window);
static int ws_fullscreen(struct windowserver* ws, struct window* window);
static int ws_set_background(struct windowserver* ws, color_t color);
static int ws_set_background_file(struct windowserver* ws, const char* path);
static int ws_draw(struct windowserver* ws);
static int ws_destroy(struct windowserver* ws);

static struct window_server_ops ws_default_ops = {
    .init = ws_init,
    .add = ws_add,
    .remove = ws_remove,
    .fullscreen = ws_fullscreen,
    .set_background = ws_set_background,
    .set_background_file = ws_set_background_file,
    .draw = ws_draw,
    .destroy = ws_destroy
};

static int ws_init(struct windowserver* ws)
{
    ERR_ON_NULL(ws);
    WS_VALIDATE(ws);

    ws->sleep_time = WINDOW_SERVER_SLEEP_TIME;
    ws->_is_fullscreen = false;

    struct windowmanager* wm = wm_new(0);
    if(wm == NULL){
        ws->ops->destroy(ws);
        return -ERROR_ALLOC;
    }
    ws->_wm = wm;
    ws->workspace = 0;

    ws->background = kalloc(VBE_SIZE());
    if(ws->background == NULL){
        wm->ops->destroy(wm);
        ws->ops->destroy(ws);
        return -ERROR_ALLOC;
    }

    SET_FlAG(ws->flags, WINDOW_SERVER_INITIALIZED);
    return 0;
}

static int ws_add(struct windowserver* ws, struct window* window)
{
    ERR_ON_NULL(ws);
    ERR_ON_NULL(window);
    WS_VALIDATE(ws);

    if(ws->_wm->ops->add(ws->_wm, window) < 0){
        return -ERROR_OPS_CORRUPTED;
    }

    return 0;
}

static int ws_remove(struct windowserver* ws, struct window* window)
{
    ERR_ON_NULL(ws);
    ERR_ON_NULL(window);
    WS_VALIDATE(ws);

    if(ws->_wm->ops->remove(ws->_wm, window) < 0){
        return -ERROR_OPS_CORRUPTED;
    }

    return 0;
}

static int ws_fullscreen(struct windowserver* ws, struct window* window)
{
    ERR_ON_NULL(ws);
    ERR_ON_NULL(window);
    WS_VALIDATE(ws);

    if(ws->_is_fullscreen){
        
         /* store and backup original window information */
        window->inner_width = window->width-16;
        window->inner_height = window->height-16;

        window->inner = ws->_tmp;
        window->pitch = window->inner_width;
        window->y = 10;
        window->x = 10;

        ws->_is_fullscreen = false;

    } else {
        window->inner_width = vbe_info->width;
        window->inner_height = vbe_info->height;

        /* save old window */
        ws->_tmp = window->inner;

        /* set new window */
        window->inner = ws->_wm->composition_buffer;
        window->pitch = vbe_info->width;

        window->y = 0;
        window->x = 0;

        ws->_is_fullscreen = true;
    }

    return 0;
}

static int ws_set_background(struct windowserver* ws, color_t color)
{
    ERR_ON_NULL(ws);
    WS_VALIDATE(ws);

    memset(ws->background, color, VBE_SIZE());

    return 0;
}

static int ws_set_background_file(struct windowserver* ws, const char* path)
{
    return 0;
}

static int __ws_key_event(struct windowserver* ws, unsigned char key)
{
    ERR_ON_NULL(ws);
    WS_VALIDATE(ws);

    if(key == 0){return 0;}

    switch (key)
    {
    case F4:{
            /* Workspace changes */
            ws->_wm->ops->remove(ws->_wm, ws->taskbar->gfx_window);

            ws->workspace = (ws->workspace+1)%WM_MAX_WORKSPACES;
            ws->_wm->ops->workspace(ws->_wm, ws->workspace);
            ws->window_changes = 1;

            ws->_wm->ops->add(ws->_wm, ws->taskbar->gfx_window);
        }
        break;
    case F10:{
            /* Fullscreen of window current in focus */
            struct window* w = ws->_wm->windows;

            ws->ops->fullscreen(ws, w);

            struct gfx_event e = {
                .data = w->inner_width,
                .data2 = w->inner_height,
                .event = GFX_EVENT_RESOLUTION
            };
            gfx_push_event(w, &e);
        }
        break;
    default: {
            /* sends keyboard event to userspace */
            struct gfx_event e = {
                .data = key,
                .event = GFX_EVENT_KEYBOARD
            };
            gfx_push_event(ws->_wm->windows, &e);
        }
        break;
    }

    return 0;
}

static int ws_draw(struct windowserver* ws)
{
    ERR_ON_NULL(ws);
    WS_VALIDATE(ws);

    /* get state variables */
    int mouse_changed = mouse_get_event(&ws->m);
    get_current_time(&ws->time);
    ws->window_changes = ws->_wm->ops->changes(ws->_wm);
    unsigned char key = kb_get_char();

    __ws_key_event(ws, key);
    
    if(ws->window_changes && !ws->_is_fullscreen){
        memcpy(ws->_wm->composition_buffer, ws->background, ws->_wm->composition_buffer_size);

        ws->_wm->ops->draw(ws->_wm, ws->_wm->windows);
    }

    return 0;

    /* Move out of this module */
    kernel_yield();

    ENTER_CRITICAL();
    /* Copy buffer over to framebuffer. */
    memcpy((uint8_t*)vbe_info->framebuffer, ws->_wm->composition_buffer, ws->_wm->composition_buffer_size);

    LEAVE_CRITICAL();

    if(mouse_changed){
        /* internal mouse event for windows */
        ws->_wm->ops->mouse_event(ws->_wm, ws->m.x, ws->m.y, ws->m.flags);
    }
    vesa_put_icon16((uint8_t*)vbe_info->framebuffer, ws->m.x, ws->m.y);

    return 0;
}

struct windowserver* ws_new()
{
    struct windowserver* ws = (struct windowserver*)kalloc(sizeof(struct windowserver));
    if(ws == NULL){
        return NULL;
    }

    ws->ops = &ws_default_ops;

    kref_get(&ws->_krefs);

    if(ws->ops->init(ws) < 0){
        kfree(ws);
        return NULL;
    }

    return ws;
}

static int ws_destroy(struct windowserver* ws)
{
    ERR_ON_NULL(ws);
    WS_VALIDATE(ws);

    if(kref_put(&ws->_krefs) > 0){
        return 0;
    }

    ws->_wm->ops->destroy(ws->_wm);

    kfree(ws);
    return 0;
}
