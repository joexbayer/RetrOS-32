#include <kthreads.h>
#include <gfx/window.h>
#include <kernel.h>

#define SECTION(w, x, y, width, height, header)\
    w->draw->box(w, x, y, width, height, 30);\
    w->draw->rect(w, x+6, y-3, strlen(header)*8, 8, 30);\
    w->draw->text(w, x+6, y-3, header, 0);

void __kthread_entry about(int argc, char* argv[])
{
    struct window* w = gfx_new_window(150, 200, 0);
    if(w == NULL){
        warningf("Failed to create window for about");
        return;
    }

    w->draw->rect(w, 0, 0, 150, 200, 30);

    w->ops->move(w, 50, 50);

    SECTION(w, 12, 12, 150-24, 200-24, "About");

    /* about page information from kernel.h */
    char* about = KERNEL_NAME "\nVersion: " KERNEL_VERSION "\nRelease: " KERNEL_RELEASE "\n\n";
    char* about2 = "RetrOS-32 is a 32-bit operating system written in C and Assembly.\n\n";

    w->draw->textf(w, 12, 30, COLOR_BLACK, about);
    w->draw->textf(w, 12, 80, COLOR_BLACK, about2);

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
EXPORT_KTHREAD(about);