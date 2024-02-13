#ifndef NCURSES
#include <libc.h>
#include <lib/syscall.h>
#else
#include <ncurses.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#endif // !NCURSES

#include "include/screen.h"
#include "include/textbuffer.h"

#define MAX_INPUT 100

int textbuffer_get_input(struct textbuffer* buffer,  char tag, char* message, int (*callback)(struct textbuffer*, char*))
{
    char input[MAX_INPUT];
    int input_len = 0;

    int box_width = 50;
    int box_height = 15;
    int screen_width = 80;
    int screen_height = 23;

    int start_x = (screen_width - box_width) / 2;
    int start_y = (screen_height - box_height) / 2;

    for (int x = start_x; x < start_x + box_width; x++) {
        for (int y = start_y; y < start_y + box_height; y++) {
            screen_put_char(x, y, ' ', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
        }
    }

    screen_draw_box(start_x, start_y, box_width, box_height, COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));


    screen_put_char(start_x+3, start_y, '[', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
	screen_put_char(start_x+4, start_y, tag, COLOR(VGA_COLOR_GREEN, VGA_COLOR_LIGHT_GREY));
	screen_put_char(start_x+5, start_y, ']', COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));

    screen_write(start_x+3, start_y+1, message, COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
    screen_write(start_x+3, start_y+2, "                 ", COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

    screen_write(start_x+3, start_y+4, "Press [Enter] to input", COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));

    while(1) {
        screen_set_cursor(start_x+3 + input_len, start_y+2);
        screen_write(start_x+3, start_y+2, input, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
        int c = screen_get_char();
        if (c == 0)
            continue;
        if (c == CTRLC)
            break;
        if (c == BACKSPACE){
            if (input_len > 0){
                input_len--;
                input[input_len] = 0;
            }
            continue;
        }
        if (c == ENTER){
            return callback(buffer, input);
        }

        if(input_len >= MAX_INPUT) {
            continue;
        }
        
        input[input_len] = c;
        input_len++;
    }
    return -1;
}
