#ifndef VBE_H
#define VBE_H

#include <stdint.h>

#define PIXELS_PER_CHAR 8
#define PIXELS_PER_ICON 16

extern struct vbe_mode_info_structure* vbe_info;
struct vbe_mode_info_structure {
   uint16_t attributes;      // deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
   uint8_t window_a;         // deprecated
   uint8_t window_b;         // deprecated
   uint16_t granularity;      // deprecated; used while calculating bank numbers
   uint16_t window_size;
   uint16_t segment_a;
   uint16_t segment_b;
   uint32_t win_func_ptr;      // deprecated; used to switch banks from protected mode without returning to real mode
   uint16_t pitch;         // number of bytes per horizontal line
   uint16_t width;         // width in pixels
   uint16_t height;         // height in pixels
   uint8_t w_char;         // unused...
   uint8_t y_char;         // ...
   uint8_t planes;
   uint8_t bpp;         // bits per pixel in this mode
   uint8_t banks;         // deprecated; total number of banks in this mode
   uint8_t memory_model;
   uint8_t bank_size;      // deprecated; size of a bank, almost always 64 KB but may be 16 KB...
   uint8_t image_pages;
   uint8_t reserved0;

   uint8_t red_mask;
   uint8_t red_position;
   uint8_t green_mask;
   uint8_t green_position;
   uint8_t blue_mask;
   uint8_t blue_position;
   uint8_t reserved_mask;
   uint8_t reserved_position;
   uint8_t direct_color_attributes;

   uint32_t framebuffer;      // physical address of the linear frame buffer; write here to draw to the screen
   uint32_t off_screen_mem_off;
   uint16_t off_screen_mem_size;   // size of memory in the framebuffer but not being displayed on the screen
   uint8_t reserved1[206];
} __attribute__ ((packed));


struct display_info {
   int width, height, pith, bbp;
   unsigned int framebuffer;
};

void vesa_put_char(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_box(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_block(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_char16(uint8_t* buffer, unsigned char c, int x, int y, int color);
void vesa_put_icon(uint8_t* buffer, int x, int y);
void vesa_write_str(uint8_t* buffer, int x, int y, const char* data, int color);
void vesa_fill(uint8_t* buffer, unsigned char color);

extern uint8_t forman[76800];

void vesa_init();

void vga_set_palette();

void vesa_put_pixel(uint8_t* buffer, int x,int y, unsigned char color);

inline void putpixel(uint8_t* buffer, int x,int y, unsigned char color, int pitch) {

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

#endif /* VBE_H */
