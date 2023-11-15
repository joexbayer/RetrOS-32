#include <colors.h>

const unsigned char vga_rgb[] = {
    0x00,0x02,0x10,0x12,0x80,0x82,0x88,0x92,0x49,0x4b,0x59,0x5b,0xc9,0xcb,0xd9,0xdb
    ,0x00,0x00,0x00,0x24,0x24,0x49,0x49,0x49,0x6d,0x6d,0x92,0x92,0xb6,0xb6,0xb6,0xdb
    ,0x03,0x23,0x63,0x83,0xc3,0xc2,0xc1,0xc0,0xc0,0xc4,0xcc,0xd0,0xd8,0x98,0x78,0x38
    ,0x18,0x18,0x19,0x1a,0x1b,0x13,0x0f,0x07,0x6f,0x6f,0x8f,0xaf,0xcf,0xce,0xce,0xcd
    ,0xcd,0xcd,0xd1,0xd5,0xd9,0xb9,0x99,0x79,0x79,0x79,0x7a,0x7a,0x7b,0x77,0x73,0x6f
    ,0x93,0xb3,0xb3,0xb3,0xd3,0xd2,0xd2,0xd2,0xd2,0xd6,0xd6,0xd6,0xda,0xba,0xba,0xba
    ,0x9a,0x9a,0x9a,0x9a,0x9b,0x97,0x97,0x97,0x01,0x01,0x21,0x41,0x41,0x41,0x40,0x40
    ,0x40,0x40,0x44,0x48,0x48,0x48,0x28,0x08,0x08,0x08,0x08,0x09,0x09,0x09,0x05,0x01
    ,0x25,0x25,0x45,0x45,0x45,0x45,0x45,0x44,0x44,0x44,0x48,0x48,0x48,0x48,0x48,0x28
    ,0x28,0x28,0x29,0x29,0x29,0x29,0x29,0x25,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49
    ,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49
    ,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x24,0x24,0x24,0x04,0x04
    ,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20
    ,0x20,0x20,0x24,0x24,0x24,0x24,0x24,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x00
    ,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24
    ,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
int size = sizeof(vga_rgb) / sizeof(vga_rgb[0]);

/* Function to split 8-bit RGB into its components */
void rgb_to_components(unsigned char color, unsigned char *r, unsigned char *g, unsigned char *b) {
    *r = (color & 0xE0) >> 5;
    *g = (color & 0x1C) >> 2;
    *b = color & 0x03;
}

/* Function to calculate the squared Euclidean distance between two colors */
float color_distance_squared(unsigned char color1, unsigned char color2) {
    unsigned char r1, g1, b1, r2, g2, b2;
    rgb_to_components(color1, &r1, &g1, &b1);
    rgb_to_components(color2, &r2, &g2, &b2);
    return (r2 - r1) * (r2 - r1) + (g2 - g1) * (g2 - g1) + (b2 - b1) * (b2 - b1);
}

/* Function to find the index of the closest color in vga_rgb */
int rgb_to_vga(unsigned char color)
{
    int closest_index = 0;
    float min_distance_squared = 255.0 * 255.0; /* Maximum possible squared distance */
    for (int i = 0; i < size; i++) {
        float distance_squared = color_distance_squared(color, vga_rgb[i]);
        if (distance_squared < min_distance_squared) {
            min_distance_squared = distance_squared;
            closest_index = i;
        }
    }
    return closest_index;
}