/**
 * @file composition.c
 * @author Joe Bayer (joexbayer)
 * @brief Window Server composition and window manage api.
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <gfx/events.h>
#include <math.h>
#include <gfx/windowserver.h>
#include <windowmanager.h>
/* prototypes */
void __kthread_entry gfx_compositor_main();

struct windowserver* ws;

/**
 * @brief Removes a window from the "wind.order" list.
 * Important keep the wind.order list intact even if removing the first element.
 * @param w 
 */
void gfx_composition_remove_window(struct window* w)
{
     while (ws == NULL || ws->_wm->state != WM_INITIALIZED);

     ws->ops->remove(ws, w);
}

/**
 * @brief Adds new window in the "wind.order" list.
 * 
 * @param w 
 */
void gfx_composition_add_window(struct window* w)
{
    while (ws == NULL || ws->_wm->state != WM_INITIALIZED);

    ws->ops->add(ws, w);
}

int gfx_set_background_color(color_t color)
{
    ERR_ON_NULL(ws);

    ws->ops->set_background(ws, color);
    return ERROR_OK;
}

int gfx_decode_background_image(const char* file)
{
    ERR_ON_NULL(ws);

    ws->ops->set_background_file(ws, file);

   return ERROR_OK;
}

int gfx_set_taskbar(pid_t pid)
{
    ERR_ON_NULL(ws);

    ws->taskbar = pcb_get_by_pid(pid);
    return ERROR_OK;

}

void __kthread_entry gfx_compositor_main()
{
    ws = ws_new();
    if(ws == NULL){
        dbgprintf("[WSERVER] Could not allocate memory for window server.\n");
        return;
    }
    
    ws->ops->set_background(ws, 3);
    ws->ops->set_background_file(ws, "/win2.img");

    dbgprintf("[WSERVER] Window server initialized.\n");

    while(1){
        ws->ops->draw(ws);
    }
}