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

int textbuffer_search_main(struct textbuffer* buffer) {
    textbuffer_get_input(buffer, 'S', "Search: ", textbuffer_search);

    return 0;
}