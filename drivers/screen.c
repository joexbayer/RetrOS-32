/**
 * @file screen.c
 * @author Joe Bayer (joexbayer)
 * @brief VGA Textmode visual driver
 * @version 0.1
 * @date 2024-02-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <screen.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <font8.h>
#include <kthreads.h>
#include <vbe.h>
#include <colors.h>
#include <screen.h>

#define VIDEO_ADDRESS 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80

/* Macro to extract the character from a VGA entry */
#define VGA_ENTRY_CHAR(vga_entry) ((vga_entry) & 0xFF)
/* Macro to extract the foreground color from a VGA entry */
#define VGA_ENTRY_FG_COLOR(vga_entry) (((vga_entry) >> 8) & 0x0F)
/* Macro to extract the background color from a VGA entry */
#define VGA_ENTRY_BG_COLOR(vga_entry) (((vga_entry) >> 12) & 0x0F)


static int screen_draw_char(struct window* w, int x, int y, unsigned char c, color_t fg, color_t bg)
{
    for (int l = 0; l < 8; l++) {
        for (int i = 8; i >= 0; i--) {
            if(c > 0 && c < 128){
                if(font8x8_basic[c][l] & (1 << i)){
                    putpixel(w->inner, (x)+i, (y)+l, fg, w->pitch);
                } else {
                    putpixel(w->inner, (x)+i, (y)+l, bg, w->pitch);
                }
            } else if(c > 179 && c < 218 ) {
                if (font8x8_box[c][l] & (1 << i)) {
                    putpixel(w->inner, (x)+i, (y)+l, fg, w->pitch);
                } else {
                    putpixel(w->inner, (x)+i, (y)+l, bg, w->pitch);
                }
            } else {
                    putpixel(w->inner, (x)+i, (y)+l, COLOR_BLUE, w->pitch);
            }
        }
    }
    return 0;
}

static int screeny(int argc, char* argv[])
{
    struct window* w = gfx_new_window(MAX_COLS*8, MAX_ROWS*8, 0);
    if(w == NULL){
        return -1;
    }
    kernel_gfx_set_title("VGA Textmode");

    start("textshell", 0, NULL);

    while (1){
        for(int k = 0; k < MAX_ROWS; k++){
            for(int j = 0; j < MAX_COLS; j++){
                uint16_t entry = scrget(j, k);
                unsigned char c = VGA_ENTRY_CHAR(entry);
                color_t fg = VGA_ENTRY_FG_COLOR(entry);
                color_t bg = VGA_ENTRY_BG_COLOR(entry);

                screen_draw_char(w, j*8, k*8, c, fg, bg);
            }
        }

        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_NONBLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_EXIT:
            return;
        case GFX_EVENT_KEYBOARD:
            scr_keyboard_add(event.data);
            break;
        default:
            break;
        }

    }

    return 0;
}
EXPORT_KTHREAD(screeny);