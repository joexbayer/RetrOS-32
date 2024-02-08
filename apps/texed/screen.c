#include <libc.h>
#include <lib/syscall.h>
#include <syscall_helper.h>
#include <args.h>

#include "../include/screen.h"

#define MAX_FMT_STR_SIZE 256

/* Macro to serialize a character and a color into a short */
/* Only platform depedent syscall needed */
int screen_put_char(int x, int y, unsigned char c, unsigned char color)
{
    return invoke_syscall(SYSCALL_SCREEN_PUT, x, y, (((unsigned short)(c) << 8) | (unsigned char)(color)));
}

char screen_get_char()
{
    return invoke_syscall(SYSCALL_SCREEN_GET, 0, 0, 0);
}

int screen_write(int x, int y, const char* str, unsigned char color)
{
    int i = 0;
    while(str[i] != '\0'){
        screen_put_char(x+i, y, str[i], color);
        i++;
    }
    return i;
}

int screen_clear_line(int y, unsigned char color)
{
    for (int x = 1; x < SCREEN_WIDTH-1; x++){
        screen_put_char(x, y, ' ', color);
    }
}

int screen_clear(int from , int to, unsigned char color)
{
    for (int y = 2; y < SCREEN_HEIGHT-2; y++){
		for (int x = 1; x < SCREEN_WIDTH-1; x++){
			screen_put_char(x, y, ' ', color);
		}
	}
}

int screen_set_cursor(int x, int y)
{
    return invoke_syscall(SYSCALL_SET_CURSOR, x, y, 0);
}


void screen_draw_box(int x, int y, int width, int height, char border_color) {
    /* Extended ASCII characters for double line box drawing */
    unsigned char top_left = 201;     /* '╔' */
    unsigned char top_right = 187;    /* '╗' */
    unsigned char bottom_left = 200;  /* '╚' */
    unsigned char bottom_right = 188; /* '╝' */
    unsigned char horizontal = 205;   /* '═' */
    unsigned char vertical = 186;     /* '║' */

    /* Draw corners */
    screen_put_char(x, y, top_left, border_color);
    screen_put_char(x + width - 1, y, top_right, border_color);
    screen_put_char(x, y + height - 1, bottom_left, border_color);
    screen_put_char(x + width - 1, y + height - 1, bottom_right, border_color);

    /* Draw top and bottom borders with connectors */
    for (int i = x + 1; i < x + width - 1; ++i) {
        screen_put_char(i, y, horizontal, border_color); /* Top border */
        screen_put_char(i, y + height - 1, horizontal, border_color); /* Bottom border */
    }

    /* Draw left, right borders, and connectors */
    for (int i = y + 1; i < y + height - 1; ++i) {
        screen_put_char(x, i, vertical, border_color); /* Left border */
        screen_put_char(x + width - 1, i, vertical, border_color); /* Right border */
    }
}

/**
 * Writes the given string with formats to screen on give location.
 * @param int x coordinate
 * @param int y coordinate
 * @param char* format string
 * @param ... variable parameters
 * @return number of bytes written
 */
int screen_printf(int x, int y, unsigned char color, char* fmt, ...)
{
	va_list args;

	int x_offset = 0;
	int written = 0;
	char str[MAX_FMT_STR_SIZE];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt){
			case '%':
				memset(str, 0, MAX_FMT_STR_SIZE);
				switch (*(fmt+1)){
					case 'd':
					case 'i': ;
						num = va_arg(args, int);
						itoa(num, str);
						screen_write(x+x_offset, y, str, color);
						x_offset += strlen(str);
						break;
                    case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);
						screen_write(x+x_offset, y, str, color);
						x_offset += strlen(str);

                        if(strlen(str) < 3){
                            int pad = 3-strlen(str);
                            for (int i = 0; i < pad; i++){
                                screen_put_char(x+x_offset, y, ' ', color);
                                x_offset++;
                            }
                        }
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						screen_write(x+x_offset, y, str, color);
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						screen_write(x+x_offset, y, str_arg, color);
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						screen_put_char(x+x_offset, y, char_arg, color);
						x_offset++;
						break;
					default:
                        break;
				}
				fmt++;
				break;
			case '\n':
				y++;
				written += x_offset;
				x_offset = 0;
				break;
			default:  
				screen_put_char(x+x_offset, y, *fmt, color);
				x_offset++;
                written++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}