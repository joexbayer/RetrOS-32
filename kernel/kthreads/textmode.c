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

#include <colors.h>
#include <font8.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <gfx/window.h>
#include <kthreads.h>
#include <screen.h>
#include <vbe.h>
#include <scheduler.h>

#define VIDEO_ADDRESS 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80

/* Macro to extract the character from a VGA entry */
#define VGA_ENTRY_CHAR(vga_entry) ((vga_entry) & 0xFF)
/* Macro to extract the foreground color from a VGA entry */
#define VGA_ENTRY_FG_COLOR(vga_entry) (((vga_entry) >> 8) & 0x0F)
/* Macro to extract the background color from a VGA entry */
#define VGA_ENTRY_BG_COLOR(vga_entry) (((vga_entry) >> 12) & 0x0F)

static int screen_draw_cursor(struct window *w, int x, int y, color_t fg) {
	int char_height = 8;  /* Height of the character */
    int total_height = 12; /* Total height of the drawing area */
    int offset = (total_height - char_height) / 2; /* Offset to center the character */

    for (int l = 0; l < total_height; l++) {
        for (int i = 0; i < 8; i++) {
            /* Check if the current pixel is within the vertical range of the character */
            if (l >= offset && l < offset + char_height) {
                /* Character drawing logic */
                if(font8x8_basic['_'][l - offset] & (1 << i)){
                    putpixel(w->inner, x + i, y + l, fg, w->pitch);
                }
            }
        }
	}
	return 0;
}

static int screen_draw_char(struct window *w, int x, int y, unsigned char c, color_t fg, color_t bg) {
    int char_height = 8;  /* Height of the character */
    int total_height = 12; /* Total height of the drawing area */
    int offset = (total_height - char_height) / 2; /* Offset to center the character */

    for (int l = 0; l < total_height; l++) {
        for (int i = 0; i < 8; i++) {
            /* Check if the current pixel is within the vertical range of the character */
            if (l >= offset && l < offset + char_height) {
                /* Character drawing logic */
                if ((c > 0 && c < 128 && font8x8_basic[c][l - offset] & (1 << i)) ||
                    (c > 179 && c < 218 && font8x8_box[c][l - offset] & (1 << i))) {
                    putpixel(w->inner, x + i, y + l, fg, w->pitch);
                } else {
                    putpixel(w->inner, x + i, y + l, bg, w->pitch);
                }
            } else {
                /* Draw the background for areas above and below the character */
                putpixel(w->inner, x + i, y + l, bg, w->pitch);
				/* if the character is a box character draw its upper row and lower row */
				if (c > 179 && c < 218) {
					if (font8x8_box[c][0] & (1 << i)) {
						putpixel(w->inner, x + i, y + l, fg, w->pitch);
					}
				}
			}
        }
    }
    return 0;
}

static int screeny(int argc, char *argv[]) {
	volatile uint16_t* buffer = scr_buffer();
	struct screen_cursor cursor = scr_get_cursor();

	/* Create a cache for the screen */
	uint16_t* cache = (uint16_t*)kalloc(MAX_ROWS * MAX_COLS * sizeof(uint16_t));
	if (cache == NULL) {
		return -1;
	}
	memset(cache, 0, MAX_ROWS * MAX_COLS * sizeof(uint16_t));

	struct window *w = gfx_new_window(MAX_COLS * 8, MAX_ROWS * 12, 0);
	if (w == NULL) {
		return -1;
	}
	kernel_gfx_set_title("VGA Textmode");

	start("textshell", 0, NULL);

	while (1) {
		cursor = scr_get_cursor();

		for (int k = 0; k < MAX_ROWS; k++) {
			for (int j = 0; j < MAX_COLS; j++) {
				uint16_t entry = buffer[k * MAX_COLS + j];
				if (entry == cache[k * MAX_COLS + j] && k != cursor.y && j != cursor.x) {
					continue;
				}

				unsigned char c = VGA_ENTRY_CHAR(entry);
				color_t fg = VGA_ENTRY_FG_COLOR(entry);
				color_t bg = VGA_ENTRY_BG_COLOR(entry);

				screen_draw_char(w, j * 8, k * 12, c, fg, bg);

				screen_draw_cursor(w, cursor.x * 8, cursor.y * 12, 0x0f);

				cache[k * MAX_COLS + j] = entry;
			}
		}

		gfx_commit();

		kernel_yield();

		struct gfx_event event;
		int ret = gfx_event_loop(&event, GFX_EVENT_NONBLOCKING);
		if (ret == -1)
			continue;

		switch (event.event) {
		case GFX_EVENT_EXIT:{
			kfree(cache);
			return -1;
		}
		case GFX_EVENT_KEYBOARD:
			scr_keyboard_add(event.data);
		break;
		default:
			break;
		}

		kernel_yield();
	}

	return 0;
}
EXPORT_KTHREAD(screeny);
