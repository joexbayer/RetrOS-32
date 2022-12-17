#include <vesa.h>
#include <vbe.h>
#include <colors.h>
#include <util.h>

struct vbe_mode_info_structure* vbe_info;

#define VESA_BG_COLOR VESA_COLOR_CYAN

static void putpixel(int x,int y, int color) {

    uint32_t* pixel_offset = y * vbe_info->pitch + (x * (vbe_info->bpp/8)) + vbe_info->framebuffer;
    *pixel_offset = color;
}

void vesa_put_char(char c, int x, int y)
{
    for (int l = 0; l < 8; l++) {

        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {
                putpixel((x*PIXELS_PER_CHAR)+i,  (y*PIXELS_PER_CHAR)+l, VESA_COLOR_BLACK);
            }
        }
    }
}

void vesa_write(int x, int y, const char* data, int size)
{
	for (int i = 0; i < size; i++)
		vesa_put_char( data[i], x+i, y);
}

void vesa_write_str(int x, int y, const char* data)
{
	vesa_write(x, y, data, strlen(data));
}



void vesa_background()
{

    for (int i = 0; i < vbe_info->height; i++)
    {
        for (int j = 0; j < vbe_info->width; j++)
        {   
            putpixel(j, i, VESA_BG_COLOR);
        }
        
    }

    char* welcome = "Welcome to VESA! In a glorious 1280x1024 resolution.";

    char fmt_str[4];

    vesa_write_str(100, 100, welcome);
    for (int i = 0; i < 128; i++)
    {
        itoa(i, fmt_str);
        vesa_write_str(0, i, fmt_str);
    }
}