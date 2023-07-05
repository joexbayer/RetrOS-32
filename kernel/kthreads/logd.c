#include <kthreads.h>
#include <gfx/window.h>
#include <terminal.h>

static struct terminal term  = {
	.head = 0,
	.tail = 0,
	.lines = 0
};

void logd_attach()
{
    terminal_attach(&term);
}

void __kthread_entry logd()
{
    struct window* w = gfx_new_window(400, 300, 0);
    if(w == NULL){
        warningf("Failed to create window for logd");
        return;
    }

    terminal_attach(&term);

    w->ops->move(w, 50, 50);

    while (1)
    {
        twritef("Hello from logd!\n");
        terminal_commit();
        kernel_sleep(100);
    }
}
EXPORT_KTHREAD(logd);