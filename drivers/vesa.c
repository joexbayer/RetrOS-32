#include <vesa.h>
#include <vbe.h>
#include <util.h>

struct vbe_mode_info_structure* vbe_info;

static void putpixel(int x,int y, int color) {

    uint32_t* pixel_offset = y * vbe_info->pitch + (x * (vbe_info->bpp/8)) + vbe_info->framebuffer;
    *pixel_offset = color;
}

void vesa_put_char(char c, int x, int y)
{
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {
                putpixel(x+i,  y+l, 0x00FF0000);
            }
        }
    }
}

void vesa_write(int x, int y, const char* data, int size)
{
	for (int i = 0; i < size; i++)
		vesa_put_char( data[i], x+(i*PIXELS_PER_CHAR), y);
}


void vesa_background()
{

    for (int i = 0; i < vbe_info->height; i++)
    {
        for (int j = 0; j < vbe_info->width; j++)
        {   
            putpixel(j, i, 0x0000FF00);
        }
        
    }

    char* welcome = "Welcome to VESA! In a glorious 1280x1024 resolution.";

    vesa_write(100, 100, welcome, strlen(welcome));
}