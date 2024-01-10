/**
 * @file msbox.c
 * @author Joe Bayer (joexbayer)
 * @brief Generic message box
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kthreads.h>
#include <scheduler.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <vbe.h>

void __kthread_entry msgbox(int argc, char* argv[])
{
    struct window* w = gfx_new_window(250, 100, GFX_NO_OPTIONS);
    if(w == NULL){
        warningf("Failed to create window for msgbox");
        return;
    }

    w->ops->move(w, 100, 100);

    while (1){
        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_EXIT:
            return;
        default:
            break;
        }

    }
}
EXPORT_KTHREAD(msgbox);