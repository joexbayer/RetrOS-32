/**
 * @file texed.c
 * @author Joe Bayer (joexbayer)
 * @brief Textmode editor
 * @version 0.1
 * @date 2024-02-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef NCURSES
#include <libc.h>
#include <lib/syscall.h>
#include <stdint.h>
#include <colors.h>
#include <math.h>
#include <fs/fs.h>
#else

#include <ncurses.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>  // For open
#include <unistd.h> // For close
#include <sys/types.h>
#include <sys/stat.h>

inline int isspace(char c)
{
    return c == ' ';
}
#endif // !NCURSES


#include "include/screen.h"
#include "include/textbuffer.h"

#define MAX_LINES 512
#define LINE_CAPACITY 78
#define MAX_VISABLE_LINES 21

/* Function prototypes */
static int textbuffer_new_line(struct textbuffer *buffer);
static int textbuffer_free_line(struct textbuffer *buffer, size_t line);
static int textbuffer_destroy(struct textbuffer *buffer);
static int textbuffer_display(const struct textbuffer *buffer, enum vga_color fg, enum vga_color bg);
static int textbuffer_handle_char(struct textbuffer *buffer, int c);
static int textbuffer_jump(struct textbuffer *buffer, unsigned int x, unsigned int y);	

static struct textbuffer_ops textbuffer_default_ops = {
		.destroy = textbuffer_destroy,
		.display = textbuffer_display,
		.put = textbuffer_handle_char,
		.jump = textbuffer_jump,
};

static struct textbuffer *textbuffer_create(void) {
	int ret;
	struct textbuffer *buffer = malloc(sizeof(struct textbuffer)); 
	if (buffer == NULL) {
		return NULL;
	}

	buffer->lines = malloc(sizeof(struct line *) * MAX_LINES);
	if (buffer->lines == NULL) {
		free(buffer);
		return NULL;
	}

	ret = textbuffer_new_line(buffer);
	if (ret < 0) {
		free(buffer->lines);
		free(buffer);
		return NULL;
	}

	buffer->syntax = malloc(sizeof(struct syntax_highlight));
	if (buffer->syntax == NULL) {
		free(buffer->lines);
		free(buffer);
		return NULL;
	}

	/* Set default syntax highlighting */
	buffer->syntax->count = 10;
	strcpy(buffer->syntax->keywords[0].keyword, (const char*)"int");
	buffer->syntax->keywords[0].color = VGA_COLOR_LIGHT_GREEN;
	strcpy(buffer->syntax->keywords[1].keyword, (const char*)"char");
	buffer->syntax->keywords[1].color = VGA_COLOR_LIGHT_GREEN;
	strcpy(buffer->syntax->keywords[2].keyword, (const char*)"void");
	buffer->syntax->keywords[2].color = VGA_COLOR_LIGHT_GREEN;
	strcpy(buffer->syntax->keywords[3].keyword, (const char*)"if");
	buffer->syntax->keywords[3].color = VGA_COLOR_LIGHT_BROWN;
	strcpy(buffer->syntax->keywords[4].keyword, (const char*)"else");
	buffer->syntax->keywords[4].color = VGA_COLOR_LIGHT_BROWN;
	strcpy(buffer->syntax->keywords[5].keyword, (const char*)"while");
	buffer->syntax->keywords[5].color = VGA_COLOR_LIGHT_BROWN;
	strcpy(buffer->syntax->keywords[6].keyword, (const char*)"for");
	buffer->syntax->keywords[6].color = VGA_COLOR_LIGHT_BROWN;
	strcpy(buffer->syntax->keywords[7].keyword, (const char*)"return");
	buffer->syntax->keywords[7].color = VGA_COLOR_LIGHT_RED;

	buffer->ops = &textbuffer_default_ops;
	buffer->cursor.x = 0;
	buffer->cursor.y = 0;
	buffer->scroll.start = 0;
	buffer->scroll.end = MAX_VISABLE_LINES;

	return buffer;
}

static int textbuffer_find(struct textbuffer *buffer, char *str) {
	int ret;
	/* find str in lines and goto line */
	for (size_t i = 0; i < buffer->line_count; i++) {
		ret = strstr(buffer->lines[i]->text, str);
		if (ret >= 0) {
			buffer->cursor.y = i;
			buffer->cursor.x = ret;
			return 0;
		}
	}

	return -1;
}

static int textbuffer_destroy(struct textbuffer *buffer) {
	for (size_t i = 0; i < buffer->line_count; i++) {
		if (buffer->lines[i]->text) {
			free(buffer->lines[i]->text);
		}
		free(buffer->lines[i]);
	}
	free(buffer->lines);
	free(buffer);

	return 0;
}

static int textbuffer_new_line(struct textbuffer *buffer) {
	if (buffer->line_count == MAX_LINES) {
		return -1;
	}

	buffer->lines[buffer->line_count] = malloc(sizeof(struct line));
	if (buffer->lines[buffer->line_count] == NULL) {
		return -1;
	}

	buffer->lines[buffer->line_count]->text = malloc(LINE_CAPACITY);
	if (buffer->lines[buffer->line_count]->text == NULL) {
		free(buffer->lines[buffer->line_count]);
		return -1;
	}

	memset(buffer->lines[buffer->line_count]->text, 0, LINE_CAPACITY);

	buffer->lines[buffer->line_count]->flags = 0;	
	buffer->lines[buffer->line_count]->length = 0;
	buffer->lines[buffer->line_count]->capacity = LINE_CAPACITY;
	buffer->line_count++;

	return 0;
}

static int textbuffer_remove_last_line(struct textbuffer *buffer) {
	if (buffer->line_count == 0) {
		return -1;
	}

	free(buffer->lines[buffer->line_count - 1]->text);
	free(buffer->lines[buffer->line_count - 1]);
	buffer->line_count--;

	return 0;
}

static int textbuffer_free_line(struct textbuffer *buffer, size_t line) {
	if (line >= buffer->line_count) {
		return -1;
	}

	/* Move the content of all lines below current up one line */
	for (size_t i = line; i < buffer->line_count - 1; i++) {
		memset(buffer->lines[i]->text, 0, buffer->lines[i]->capacity);
		memcpy(buffer->lines[i]->text, buffer->lines[i + 1]->text, buffer->lines[i + 1]->length);
		buffer->lines[i]->length = buffer->lines[i + 1]->length;
	}

	/* Remove the last line */
	free(buffer->lines[buffer->line_count - 1]->text);
	free(buffer->lines[buffer->line_count - 1]);
	buffer->line_count--;

	return 0;
}
#define HAS_FLAG(flags, flag) (flags & flag)

static int textbuffer_save_file(struct textbuffer *buffer, const char *filename) {
	char* file = malloc(2048);
	if (file == NULL) {
		return -1;
	}

	size_t len = 0;
	for (size_t i = 0; i < buffer->line_count; i++) {
		if (buffer->lines[i]->text) {
			memcpy(file + len, buffer->lines[i]->text, buffer->lines[i]->length);
			/* Add newline if not extension */
			if (!HAS_FLAG(buffer->lines[i]->flags, LINE_FLAG_EXTENSION)) {
				file[len + buffer->lines[i]->length] = ENTER;
				len++;
			}	

			len += buffer->lines[i]->length;
		}
	}
	
#ifndef NCURSES
	int fd = open(buffer->filename, FS_FILE_FLAG_CREATE | FS_FILE_FLAG_READ | FS_FILE_FLAG_WRITE);
#else
	int fd = open(buffer->filename, O_WRONLY | O_CREAT, 0644);
#endif // !NCURSES
	if (fd < 0) {
		free(file);
		return -1;
	}

	int ret = write(fd, file, len);
	if (ret < 0) {
		fclose(fd);
		free(file);
		return -1;
	}

	fclose(fd);
	free(file);

	return 0;
}

static int textbuffer_load_file(struct textbuffer *buffer, const char *filename){
	char* file = malloc(2048);
	if (file == NULL) {
		return -1;
	}

#ifndef NCURSES
	int fd = open(filename, FS_FILE_FLAG_READ | FS_FILE_FLAG_WRITE);
#else
	int fd = open(filename, O_RDONLY);
#endif // !NCURSES

	if (fd < 0) {
		return -1;
	}
	int ret = read(fd, file, 2048);
	if (ret < 0) {
		fclose(fd);
		return -1;
	}

#ifndef NCURSES
	fclose(fd);
#else
	close(fd);
#endif // !NCURSES

	/* set filename */
	strcpy(buffer->filename, filename);	
	screen_printf(SCREEN_WIDTH/2 - strlen(filename)/2, 1, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE), filename);
	
	size_t len = ret;
	for (size_t i = 0; i < len; i++) {
		textbuffer_handle_char(buffer, file[i]);
	}

	textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	return 0;
}

static int textbuffer_jump(struct textbuffer *buffer, unsigned int x, unsigned int y) {
    buffer->cursor.y = y;
	buffer->cursor.x = x;

	if(buffer->cursor.y > 20) {
		buffer->scroll.start = buffer->cursor.y - 20;
	}

	textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	return 0;
}

/* Function to find a keyword in the text and return its color, or default color if not found */
static enum vga_color get_keyword_color(const struct textbuffer *buffer, const char *text, size_t text_len, size_t *keyword_length) {
    for (int i = 0; i < buffer->syntax->count; i++) {
        const char *keyword = buffer->syntax->keywords[i].keyword;
        size_t keyword_len = strlen(keyword);

        if (text_len >= keyword_len && strncmp(text, keyword, keyword_len) == 0 &&
            (text_len == keyword_len || isspace(text[keyword_len]))) {
            *keyword_length = keyword_len;
            return buffer->syntax->keywords[i].color;
        }
    }

    *keyword_length = 0; 
    return VGA_COLOR_WHITE;
}

static int textbuffer_print_line(struct textbuffer *buffer, int x, int y, size_t line) {
    if (line >= buffer->line_count) {
        return -1;
    }

    struct line *l = buffer->lines[line];
    for (size_t i = 0; i < l->length;) {  
        size_t remaining_length = l->length - i;
        size_t keyword_len = 0;
        enum vga_color color = get_keyword_color(buffer, &l->text[i], remaining_length, &keyword_len);

        if (keyword_len > 0) {
            for (size_t j = 0; j < keyword_len; j++) {
                screen_put_char(x + i + j, y, l->text[i + j], COLOR(color, VGA_COLOR_BLUE));
            }
            i += keyword_len;
        } else {
            screen_put_char(x + i, y, l->text[i], COLOR(color, VGA_COLOR_BLUE));
            i++;
        }
    }

    return 0;
}



/* Function to display the content of the text buffer on the screen */
static int textbuffer_display(const struct textbuffer *buffer, enum vga_color fg, enum vga_color bg) {
	uint32_t x_start = 0;
	uint32_t last_line_y = 0;
	uint32_t y_start = buffer->scroll.start;

	/* Clear the screen before displaying new content */
	//screen_clear(0, 0, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

	/* fill top row with light grey */
	for (size_t i = 0; i < 80; i++) {
		screen_put_char(i, 0, ' ', COLOR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
	}
	screen_printf(0, 0, COLOR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), " File    Edit    Search                                                    Help");

	/* draw mode */
	screen_put_char(3, 1, '[', COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
	screen_put_char(4, 1, 'R', COLOR(VGA_COLOR_GREEN, VGA_COLOR_BLUE));
	screen_put_char(5, 1, ']', COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

	/* write from start to end, or line_count */
	for (size_t i = 0; i + y_start < buffer->line_count && i < buffer->scroll.end; i++) {
		/* Calculate the position for each line */
		uint32_t x = x_start;
		uint32_t y = y_start + i;
		last_line_y = 2 + i;

		/* Write the line to the screen */
		// if (!(buffer->lines[y]->flags & LINE_FLAG_DIRTY)) {
		//	continue;
		//}

		screen_clear_line(2 + i, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
		if (buffer->cursor.y == y) {
			//screen_printf(1+x, 2 + i, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE) , "%s %c", buffer->lines[y]->text, 27);
		} else {
			//screen_printf(1+x, 2 + i, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE), "%s", buffer->lines[y]->text);
		}
		textbuffer_print_line(buffer, 1+x, 2 + i, y);

		/* Clear DIRTY flag */
		buffer->lines[y]->flags &= ~LINE_FLAG_DIRTY;
	}

	 /* Clear the line after the last actual line */
    if (last_line_y < MAX_VISABLE_LINES) {
        screen_clear_line(last_line_y + 1, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    }

	screen_set_cursor(buffer->cursor.x+1, 2+buffer->cursor.y - buffer->scroll.start);

	/* fill bottom row with light grey */
	for (size_t i = 0; i < 80; i++) {
		screen_put_char(i, 24, ' ', COLOR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
	}

	/* write stats at the bottom */
	screen_printf(0, 24, COLOR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), "lc: %d, x: %d, y: %d,          Save CTRL-S, Command CTRL-C, Search CTRL-F", buffer->line_count, buffer->cursor.x, buffer->cursor.y);

	return 0;
}

/* Function to handle keyboard input of a char */
static int textbuffer_handle_char(struct textbuffer *buffer, int c) {
	int ret;

	/* Handle the backspace key */
	if (c == BACKSPACE) {
		/* If we are at the end of the line */
		if (buffer->cursor.x == 0) {
			/* If the cursor is at the beginning of the first line, do nothing */
			if (buffer->cursor.y == 0) {
				return -1;
			}

			/* Move the rest of the current line up if there is space. */
			if (buffer->lines[buffer->cursor.y]->length > 0 && buffer->lines[buffer->cursor.y - 1]->capacity - buffer->lines[buffer->cursor.y - 1]->length > buffer->lines[buffer->cursor.y]->length) {
				memcpy(buffer->lines[buffer->cursor.y - 1]->text + buffer->lines[buffer->cursor.y - 1]->length, buffer->lines[buffer->cursor.y]->text, buffer->lines[buffer->cursor.y]->length);
			}
			buffer->cursor.x = buffer->lines[buffer->cursor.y - 1]->length;

			/* update new length */
			buffer->lines[buffer->cursor.y - 1]->length += buffer->lines[buffer->cursor.y]->length;

			textbuffer_free_line(buffer, buffer->cursor.y);

			buffer->cursor.y--;

			/* mark as dirty */
			buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
		} else {
			/* Remove the character before the cursor */
			if (buffer->lines[buffer->cursor.y] == NULL) {
				buffer->cursor.x = 0;
				return -1;
			}

			/* If x is behind length, then we move content back */
			if (buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
				for (size_t i = buffer->cursor.x; i < buffer->lines[buffer->cursor.y]->length; i++) {
					buffer->lines[buffer->cursor.y]->text[i - 1] = buffer->lines[buffer->cursor.y]->text[i];
				}
				buffer->lines[buffer->cursor.y]->text[buffer->lines[buffer->cursor.y]->length - 1] = '\0';
				buffer->lines[buffer->cursor.y]->length--;
				buffer->cursor.x--;

				/* mark as dirty */
				buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
				return -1;
			}

			/* If x is at length, then we just remove the last character */
			buffer->lines[buffer->cursor.y]->text[buffer->cursor.x - 1] = 0;
			buffer->lines[buffer->cursor.y]->length--;
			buffer->cursor.x--;

			/* mark as dirty */
			buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
		}
	} else if (c == '\n') {
		/* If the cursor is at the end of the line, create a new line */
		if (buffer->cursor.x == buffer->lines[buffer->cursor.y]->length) {
			/* If the buffer is full, do nothing */
			if (buffer->line_count == MAX_LINES) {
				return -1;
			}

			ret = textbuffer_new_line(buffer);
			if (ret < 0) {
				return -1;
			}

			if (buffer->cursor.y != buffer->line_count - 1) {
				/* Move down the content of all lines below current */
				for (size_t i = buffer->line_count - 1; i > buffer->cursor.y + 1; i--) {
					memset(buffer->lines[i]->text, 0, buffer->lines[i]->capacity);
					memcpy(buffer->lines[i]->text, buffer->lines[i - 1]->text, buffer->lines[i - 1]->length);
					buffer->lines[i]->length = buffer->lines[i - 1]->length;

					/* mark as dirty */
					buffer->lines[i]->flags |= LINE_FLAG_DIRTY;
				}
				/* clear next line */
				memset(buffer->lines[buffer->cursor.y + 1]->text, 0, buffer->lines[buffer->cursor.y + 1]->capacity);
				buffer->lines[buffer->cursor.y + 1]->length = 0;
			}

			/* Move the cursor to the beginning of the new line */
			buffer->cursor.x = 0;
			buffer->cursor.y++;

			/* mark as dirty */
			buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
		} else {
			/* Create a new line */
			ret = textbuffer_new_line(buffer);
			if (ret < 0) {
				return -1;
			}

			/* Move down the content of all lines below current */
			for (size_t i = buffer->line_count - 1; i > buffer->cursor.y + 1; i--) {
				memcpy(buffer->lines[i]->text, buffer->lines[i - 1]->text, buffer->lines[i - 1]->length);
				buffer->lines[i]->length = buffer->lines[i - 1]->length;

				/* mark as dirty */
				buffer->lines[i]->flags |= LINE_FLAG_DIRTY;
			}

			/* clear next line */
			memset(buffer->lines[buffer->cursor.y + 1]->text, 0, buffer->lines[buffer->cursor.y + 1]->capacity);

			/* Move the content from x to length down one line */
			for (size_t i = buffer->lines[buffer->cursor.y]->length; i > buffer->cursor.x; i--) {
				buffer->lines[buffer->cursor.y + 1]->text[i - buffer->cursor.x - 1] = buffer->lines[buffer->cursor.y]->text[i - 1];
			}

			/* Set the length of the new line */
			buffer->lines[buffer->cursor.y + 1]->length = buffer->lines[buffer->cursor.y]->length - buffer->cursor.x;

			memset(buffer->lines[buffer->cursor.y]->text + buffer->cursor.x, 0, buffer->lines[buffer->cursor.y]->capacity - buffer->cursor.x);

			/* Set the length of the old line */
			buffer->lines[buffer->cursor.y]->length = buffer->cursor.x;

			/* Move the cursor to the beginning of the new line */
			buffer->cursor.x = 0;
			buffer->cursor.y++;

			/* mark as dirty */
			buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
		}
		if (buffer->cursor.y >= buffer->scroll.end + buffer->scroll.start) {
			buffer->scroll.start++;
		}
	} else if (c == ARROW_RIGHT || c == ARROW_LEFT || c == ARROW_UP || c == ARROW_DOWN) {
		switch (c) {
		case ARROW_RIGHT:
			if (buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
				buffer->cursor.x++;
			}
			break;
		case ARROW_LEFT:
			if (buffer->cursor.x > 0) {
				buffer->cursor.x--;
			}
			break;
		case ARROW_UP:
			if (buffer->cursor.y > 0) {
				buffer->cursor.y--;
				if (buffer->cursor.x > buffer->lines[buffer->cursor.y]->length) {
					buffer->cursor.x = buffer->lines[buffer->cursor.y]->length;
				}
				if (buffer->scroll.start > 0 &&
						buffer->cursor.y < buffer->scroll.start) {
					buffer->scroll.start--;
				}
			}
			break;
		case ARROW_DOWN:
			if (buffer->cursor.y < buffer->line_count - 1) {
				buffer->cursor.y++;
				if (buffer->cursor.x > buffer->lines[buffer->cursor.y]->length) {
					buffer->cursor.x = buffer->lines[buffer->cursor.y]->length;
				}

				if (buffer->cursor.y >= buffer->scroll.end + buffer->scroll.start) {
					buffer->scroll.start++;
				}
			}
			break;
		}
		//screen_set_cursor(buffer->cursor.x, 1+buffer->cursor.y);
	} else {
		/* If the buffer is full, do nothing */
		if (buffer->lines[buffer->cursor.y]->length == buffer->lines[buffer->cursor.y]->capacity) {
			/* Create new line, but flag it as extension */
			textbuffer_handle_char(buffer, '\n');
			buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_EXTENSION;
		}

		/* If x is behind length, then we move content forward */
		if (buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
			/* Move the content from x to end forward one character */
			for (size_t i = buffer->lines[buffer->cursor.y]->length; i > buffer->cursor.x; i--) {
				buffer->lines[buffer->cursor.y]->text[i] = buffer->lines[buffer->cursor.y]->text[i - 1];

				/* mark as dirty */
				buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
			}
		}

		/* Insert the character at the cursor position */
		buffer->lines[buffer->cursor.y]->text[buffer->cursor.x] = c;
		buffer->lines[buffer->cursor.y]->length++;
		buffer->cursor.x++;

		/* mark as dirty */
		buffer->lines[buffer->cursor.y]->flags |= LINE_FLAG_DIRTY;
	}

	return 0;
}

int main(int argc, char *argv[]) {


#ifdef NCURSES
    initscr();

    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        return -1;
    }

    start_color();

    noecho();
    keypad(stdscr, TRUE);
#endif // !NCURSES

	int ret;
	int c;
	struct textbuffer *buffer = textbuffer_create();
	if (buffer == NULL) {
		return -1;
	}

	screen_clear(0, 0, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

	screen_draw_box(0, 1, 80, 23, COLOR(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
	if(argc > 1) {
		textbuffer_load_file(buffer, argv[1]);
		strcpy(buffer->filename, argv[1]);
	}

	textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	while (1) {
		c = screen_get_char();
		if (c == 0)
			continue;
		if (c == CTRLC){
				ret = textbuffer_command_main(buffer);
				textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
				if(ret == -117) break;
				continue;
			}
		if (c == CTRLS){
			textbuffer_save_file(buffer, buffer->filename);
			continue;
		}

		if (c == CTRLF){
			ret = textbuffer_search_main(buffer);
			textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
			if(ret < 0)
				screen_write(0, 24, "Search failed", COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY));
			continue;
		}
			
		textbuffer_handle_char(buffer, c);
		textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	}

	textbuffer_destroy(buffer);


	#ifdef NCURSES
	endwin();
	#endif // !NCURSES

	return 0;
}
