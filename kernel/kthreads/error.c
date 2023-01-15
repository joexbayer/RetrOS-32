#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <scheduler.h>


void error_main()
{
    gfx_new_window(200, 50);
    gfx_draw_text(5, 5, "Page Fault", VESA8_COLOR_LIGHT_RED);
    gfx_draw_text(5, 15, "Process has been killed.", VESA8_COLOR_LIGHT_RED);
    gfx_commit();
    while(1)
    {
        yield();
    }
}