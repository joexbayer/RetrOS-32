#include <kthreads.h>
#include <scheduler.h>
#include <pcb.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <terminal.h>
#include <logd.h>
#include <vbe.h>

static struct terminal term  = {
	.head = 0,
	.tail = 0,
	.lines = 0
};

void logd_attach_by_pid(int pid)
{
    struct pcb* pcb = pcb_get_by_pid(pid);
    if(pcb->state == STOPPED){
        warningf("Failed to attach logd to pid %d", pid);
        return;
    }

    pcb->term = &term;
}

void __kthread_entry logd(int argc, char* argv[])
{
    struct window* w = gfx_new_window(400, 300, 0);
    if(w == NULL){
        warningf("Failed to create window for logd");
        return;
    }

    w->ops->move(w, 50, 50);

    while (1)
    {
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
EXPORT_KTHREAD(logd);