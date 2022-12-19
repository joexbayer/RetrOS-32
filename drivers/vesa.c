#include <vesa.h>
#include <vbe.h>
#include <colors.h>
#include <util.h>
#include <font8.h>

struct vbe_mode_info_structure* vbe_info;

#define VESA_BG_COLOR VESA8_COLOR_DARK_TURQUOISE

/* http://www.piclist.com/tecHREF/datafile/charset/extractor/charset_extractor.html */

unsigned short b8to16(unsigned char c)
{
    return (((unsigned short)c)<<8 ) | c;
}

inline void putpixel(int x,int y, unsigned char color) {

    uint8_t* pixel_offset = (uint8_t*) (y * vbe_info->pitch + (x * (vbe_info->bpp/8)) + vbe_info->framebuffer);
    *pixel_offset = color;
}


void vesa_put_pixel(int x,int y, unsigned char color)
{
    putpixel(x , y, color);
}

void vesa_put_char(unsigned char c, int x, int y)
{
    for (int l = 0; l < 8; l++) {

        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {
                putpixel((x*PIXELS_PER_CHAR)+i,  (y*PIXELS_PER_CHAR)+l, VGA_COLOR_WHITE);
            } else {
                putpixel((x*PIXELS_PER_CHAR)+i,  (y*PIXELS_PER_CHAR)+l, VGA_COLOR_BLACK);
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

inline void vesa_line_horizontal(int x, int y, int length, int color)
{
    for (int i = x; i < (x+length); i++)
        putpixel(i, y, color);
}

inline void vesa_line_vertical(int x, int y, int length, int color)
{
    for (int i = y; i < (y+length); i++)
        putpixel(x, i, color);
}


inline void vesa_fillrect(int x, int y, int w, int h, int color) {
    int i, j;
    
    for (j = y; j < (y+h); j++)
        for (i = x; i < (x+w); i++)
            putpixel(i, j, color);
}

void vesa_inner_box(int x, int y, int w, int h)
{
    vesa_fillrect(x, y, w, h, VESA8_COLOR_LIGHT_GRAY3);

    vesa_line_horizontal(x, y, w, VESA8_COLOR_DARK_GRAY2);
    vesa_line_horizontal(x, y+h, w, VESA8_COLOR_LIGHT_GRAY1);

    vesa_line_vertical(x, y, h, VESA8_COLOR_DARK_GRAY2);
    vesa_line_vertical(x+w, y, h, VESA8_COLOR_LIGHT_GRAY1);

}

void vesa_fill(unsigned char color)
{
    for (int i = 0; i < vbe_info->height; i++)
        for (int j = 0; j < vbe_info->width; j++)
            putpixel(j, i, color);


    vesa_fillrect(0, 480-25, 640, 25, VESA8_COLOR_LIGHT_GRAY3);
    vesa_line_horizontal(0, 480-25, 640, VESA8_COLOR_LIGHT_GRAY1);
    vesa_line_horizontal(0, 480-26, 640, VESA8_COLOR_LIGHT_GRAY1);
}

void vesa_background()
{

    char* welcome = "Welcome to VESA! In a glorious 640x480x8 resolution.";
    vesa_write_str(1, 1, welcome);

    vesa_inner_box(638-80, 480-22, 80, 19);
}