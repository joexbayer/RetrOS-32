#include <scheduler.h>
#include <gfx/gfxlib.h>
#include <vbe.h>

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240

void image_viewer()
{
    struct gfx_window* window = gfx_new_window(IMAGE_WIDTH, IMAGE_HEIGHT);

    for (int i = 0; i < 320; i++){
        for (int j = 0; j < 240; j++){
            putpixel(window->inner, j, i, forman[j*IMAGE_WIDTH + i], window->inner_height);
        }
    }

    while (1){kernel_yield();}    
    return 0;
}