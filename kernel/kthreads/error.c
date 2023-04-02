#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <scheduler.h>


void error_main()
{
    gfx_new_window(200, 50);
    __gfx_draw_text(5, 5, "Page Fault", COLOR_BRIGHT_RED);
    __gfx_draw_text(5, 15, "Process has been killed.", COLOR_BRIGHT_RED);
    gfx_commit();
    while(1){
        kernel_yield();
    }
}