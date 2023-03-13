#ifndef DFA8C135_4052_4480_8A44_09EA9D67997D
#define DFA8C135_4052_4480_8A44_09EA9D67997D

#include <stdint.h>
#include <vbe.h>

#define PIXELS_PER_CHAR 8
#define PIXELS_PER_ICON 16

void vesa_put_char(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_char16(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_icon(uint8_t* buffer, int x, int y);
void vesa_write_str(uint8_t* buffer, int x, int y, const char* data, int color);
void vesa_fill(uint8_t* buffer, unsigned char color);

extern uint8_t forman[76800];

void vesa_init();

void vesa_put_pixel(uint8_t* buffer, int x,int y, unsigned char color);

inline void putpixel(uint8_t* buffer, int x,int y, char color, int pitch) {

    uint8_t* pixel_offset = (uint8_t*) (y * pitch + (x * (vbe_info->bpp/8)) + buffer);
    *pixel_offset = color;
}

inline unsigned char* getpixel(uint8_t* buffer, int x,int y, int pitch) {

    uint8_t* pixel_offset = (uint8_t*) (y * pitch + (x * (vbe_info->bpp/8)) + buffer);
    return pixel_offset;
}

inline void vesa_line_horizontal(uint8_t* buffer, int x, int y, int length, int color)
{
    for (int i = x; i < (x+length); i++)
        putpixel(buffer, i, y, color, vbe_info->pitch);
}

inline void vesa_line_vertical(uint8_t* buffer, int x, int y, int length, int color)
{
    for (int i = y; i < (y+length); i++)
        putpixel(buffer, x, i, color, vbe_info->pitch);
}

void vesa_fillrect(uint8_t* buffer, int x, int y, int w, int h, int color);

int vesa_printf(uint8_t* buffer, int32_t x, int32_t y, int color, char* fmt, ...);
void vesa_inner_box(uint8_t* buffer, int x, int y, int w, int h);
void vesa_init();

void vesa_put_icon16(uint8_t* buffer, int x, int y);
void vesa_put_icon32(uint8_t* buffer, int x, int y);

#endif /* DFA8C135_4052_4480_8A44_09EA9D67997D */
