#include <vesa.h>
#include <vbe.h>
#include <colors.h>
#include <util.h>

struct vbe_mode_info_structure* vbe_info;

#define VESA_BG_COLOR 172

unsigned short b8to16(unsigned char c)
{
    return (((unsigned short)c)<<8 ) | c;
}

static void putpixel(int x,int y, unsigned char color) {

    uint8_t* pixel_offset = y * vbe_info->pitch + (x * (vbe_info->bpp/8)) + vbe_info->framebuffer;
    *pixel_offset = color;
}

void vesa_put_char(char c, int x, int y)
{
    for (int l = 0; l < 8; l++) {

        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {
                putpixel((x*PIXELS_PER_CHAR)+i,  (y*PIXELS_PER_CHAR)+l, VGA_COLOR_WHITE);
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

    int current_color = 0;
    int tick = 0;
    for (int i = 0; i < vbe_info->height; i++)
    {
        if(tick >  vbe_info->width/256){
            tick = 0;
            current_color += 4;
        }
        for (int j = 0; j < vbe_info->width; j++)
        {   
            putpixel(j, i, current_color % 255);
        }
        tick++;
    }
    
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {   
            putpixel(30+j, 30+i, VGA_COLOR_BLACK);
        }
    }

    

    char* welcome = "Welcome to VESA! In a glorious 640x480x8 resolution.";
    vesa_write_str(1, 1, welcome);

    char fmt_str[4];
    for (int i = 0; i < vbe_info->height/PIXELS_PER_CHAR; i++)
    {
        itoa(i, fmt_str);
        vesa_write_str(0, i, fmt_str);
    }
}