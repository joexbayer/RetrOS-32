#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <scheduler.h>


void error_main()
{
    gfx_new_window(200, 50, 0);
    kernel_gfx_draw_text(5, 5, "Page Fault", COLOR_BOX_LIGHT_RED);
    kernel_gfx_draw_text(5, 15, "Process has been killed.", COLOR_BOX_LIGHT_RED);
    gfx_commit();
    while(1){
        kernel_yield();
    }
}