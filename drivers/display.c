#include <lib/display.h>
#include <vbe.h>

struct display_driver {
    int width;
    int height;
    int size;
    int bpp;
} current_display_driver;

int display_get_info(struct display_info* info)
{
    struct display_info display = {
        .bpp = vbe_info->bpp,
        .width = vbe_info->width,
        .height = vbe_info->height,
        .color = 255,
        .name = "VESA"
    };

    *info = display;

    return 0;
}

