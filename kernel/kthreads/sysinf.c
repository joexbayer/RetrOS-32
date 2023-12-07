#include <kthreads.h>
#include <gfx/window.h>
#include <kutils.h>
#include <util.h>

#define WIDTH 175
#define HEIGHT 225

void __kthread_entry sysinf(int argc, char* argv[])
{   
    struct gfx_theme* theme;
    
    struct window* w = gfx_new_window(WIDTH, HEIGHT, 0);
    if(w == NULL){
        warningf("Failed to create window for sysinf");
        return;
    }

    w->ops->move(w, 50, 50);
    while (1){
        theme = kernel_gfx_current_theme();

        w->draw->rect(w, 0, 0, WIDTH, HEIGHT, 30);

        gfx_draw_contoured_box(10, 0, WIDTH-20, 30, 30);

        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;
        switch (event.event){
        case GFX_EVENT_EXIT:
            kernel_exit();
            break;
        case GFX_EVENT_KEYBOARD:
            /* key in event.data */
            break;
        case GFX_EVENT_MOUSE:
            /* mouse x/y in data and data2 */
            break;
        default:
            break;
        }

    }
}
EXPORT_KTHREAD(kclock);