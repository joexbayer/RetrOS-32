#include <libc.h>
#include <lib/syscall.h>

#include "include/screen.h"
#include "include/textbuffer.h"

#define MAX_SEARCH 100

static int textbuffer_search(struct textbuffer* buffer, char* search) {
    int ret;
	/* find str in lines and goto line */
	for (size_t i = 0; i < buffer->line_count; i++) {
		ret = strstr(buffer->lines[i]->text, search);
		if (ret >= 0) {
			buffer->cursor.x = ret;
            buffer->ops->jump(buffer, ret, i);
			return 0;
		}
	}
	return -1;
}

static int textbuffer_search_box(struct textbuffer* buffer) {
    
    char search[MAX_SEARCH] = {0};
    int search_len = 0;

    for(int x = 10; x < 60; x++) 
        for(int y = 5; y < 20; y++)
            screen_put_char(x, y, ' ', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));

    screen_draw_box(10, 5, 50, 15, COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));

    screen_put_char(13, 5, '[', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
	screen_put_char(14, 5, 'S', COLOR(VGA_COLOR_GREEN, VGA_COLOR_LIGHT_GREY));
	screen_put_char(15, 5, ']', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));

    screen_write(13, 6, "Search: ", COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
    screen_write(13, 7, "          ", COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

    while(1) {
        screen_set_cursor(13 + search_len, 7);
        screen_write(13, 7, search, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
        unsigned char c = screen_get_char();
        if (c == 0)
            continue;
        if (c == CTRLC)
            break;
        if (c == '\b'){
            if (search_len > 0){
                search_len--;
                search[search_len] = 0;
            }
            continue;
        }
        if (c == '\n'){
            return textbuffer_search(buffer, search);
        }

        search[search_len] = c;
        search_len++;
    }
    return -1;
}

int textbuffer_search_main(struct textbuffer* buffer) {
    textbuffer_search_box(buffer);
    return 0;
}