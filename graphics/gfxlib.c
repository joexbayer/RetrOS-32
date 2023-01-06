/**
 * @file gfxlib.c
 * @author Joe Bayer (joexbayer)
 * @brief GFX library for rending to a framebuffer.
 * @version 0.1
 * @date 2023-01-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <pcb.h>
#include <font8.h>
#include <vesa.h>

/**
 * @brief Draws a rectangle onto the inner framebufferfor currently running process.
 * 
 * @param x coordinate
 * @param y coordiante
 * @param width of rectangle
 * @param height of rectangle
 * @param color
 * @return int 0 on success, less than 0 on error
 */
int gfx_draw_rectangle(int x, int y, int width, int height, char color)
{
    if(current_running->gfx_window == NULL)
        return -1;

    if(x < 0 || y < 0 || x+width > current_running->gfx_window->width || y+height > current_running->gfx_window->height)
        return -2;

    CLI();
    int i, j;
    for (j = y; j < (y+height); j++)
        for (i = x; i < (x+width); i++)
            putpixel(current_running->gfx_window->inner, j, i, color, current_running->gfx_window->height-18);
    STI();

    current_running->gfx_window->changed = 1;
    return 0;
}

/**
 * @brief Draws a character to the framebuffer of currently running process.
 * 
 * @param x coordinate
 * @param y coordiante
 * @param c character
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int gfx_draw_char(int x, int y, char c, char color)
{

    if(current_running->gfx_window == NULL)
        return -1;

    CLI();
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if (font8x8_basic[c][l] & (1 << i)) {

                if((x)+i < 0 || (y)+l < 0 || (x)+i > current_running->gfx_window->width || (y)+l > current_running->gfx_window->height)
                    continue;

                putpixel(current_running->gfx_window->inner, (y)+l, (x)+i, color, current_running->gfx_window->height-18);
            }
        }
    }
    STI();
    current_running->gfx_window->changed = 1;

    return 0;
}

/**
 * @brief gfx_draw_char wrapper for strings
 * 
 * @param x 
 * @param y 
 * @param str string to write.
 * @param color 
 * @return int 0 on success, less than 0 on error.
 */
int gfx_draw_text(int x, int y, char* str, char color)
{
    if(current_running->gfx_window == NULL)
        return -1;

    for (int i = 0; i < strlen(str); i++)
    {
        gfx_draw_char(x+(i*8), y, str[i], color);
    }

    return 0;
}