#include <scheduler.h>
#include <gfx/gfxlib.h>
#include <kthreads.h>
#include <vbe.h>

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240

void image()
{
    struct gfx_window* window = gfx_new_window(IMAGE_WIDTH, IMAGE_HEIGHT);
    
    for (int i = 0; i < 320; i++){
        for (int j = 0; j < 240; j++){
            putpixel(window->inner, i, j, forman[j*IMAGE_WIDTH + i], window->pitch);
        }
    }

    while (1){kernel_yield();}    
    return 0;
}
EXPORT_KTHREAD(image);