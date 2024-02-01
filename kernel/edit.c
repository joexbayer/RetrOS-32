#include <kernel.h>
#include <kutils.h>
#include <memory.h>
#include <screen.h>
#include <colors.h>
#include <ksyms.h>
#include <kthreads.h>
#include <keyboard.h>

#define MAX_LINES 512

struct line {
    char *text;
    size_t length;
    size_t capacity;
};

struct textbuffer {
    struct line **lines;
    size_t line_count;
    struct cursor {
        size_t x;
        size_t y;
    } cursor;
};

static struct textbuffer *textbuffer_create(void)
{
    struct textbuffer *buffer = create(struct textbuffer);
    buffer->lines = kalloc(sizeof(struct line*) * MAX_LINES);
    buffer->lines[0]->text = kalloc(32);
    buffer->line_count = 1;
    buffer->lines[0]->length = 0;
    buffer->lines[0]->capacity = 32;
    buffer->cursor.x = 0;
    buffer->cursor.y = 0;
    return buffer;
}

static void textbuffer_destroy(struct textbuffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; i++) {
        if (buffer->lines[i]->text) {
            free(buffer->lines[i]->text);
        }
        free(buffer->lines[i]);
    }
    free(buffer->lines);
    free(buffer);
}

static int textbuffer_new_line(struct textbuffer *buffer)
{
    if (buffer->line_count == MAX_LINES) {
        return -1;
    }

    buffer->lines[buffer->line_count] = kalloc(sizeof(struct line));
    buffer->lines[buffer->line_count]->text = kalloc(32);
    buffer->lines[buffer->line_count]->length = 0;
    buffer->lines[buffer->line_count]->capacity = 32;
    buffer->line_count++;

    return 0;
}

/* Function to display the content of the text buffer on the screen */
static void textbuffer_display(const struct textbuffer *buffer, enum vga_color fg, enum vga_color bg) {
    uint8_t color = (bg << 4) | fg;
    uint32_t x_start = 0;
    uint32_t y_start = 0;

    /* Clear the screen before displaying new content */
    scr_clear();

    for (size_t i = 0; i < buffer->line_count; i++) {
        /* Calculate the position for each line */
        int32_t x = x_start;
        int32_t y = y_start + i;

        /* Write the line to the screen */
        if (buffer->lines[i]->text) {
            scrprintf(x, y, "%s", buffer->lines[i]->text);
        }
    }

    /* Set the cursor to the end of the text */
    if (buffer->line_count > 0) {
        screen_set_cursor(x_start, y_start + buffer->line_count - 1);
    }

    screen_set_cursor(buffer->cursor.x == 0 ? buffer->cursor.x : buffer->cursor.x -1, buffer->cursor.y);
    /* write stats at the bottom */
    scrprintf(0, 24, "line_count: %d, x: %d, y: %d", buffer->line_count, buffer->cursor.x, buffer->cursor.y);
}

/* Function to handle keyboard input of a char */
static void textbuffer_handle_char(struct textbuffer *buffer, unsigned char c) {
    /* Handle the backspace key */
    if (c == '\b') {
        /* If the cursor is at the beginning of the line, go to the previous line */
        if (buffer->cursor.x == 0) {
            /* If the cursor is at the beginning of the first line, do nothing */
            if (buffer->cursor.y == 0 ) {
                return;
            }

            /* TODO: move rest of line up */
            if(buffer->lines[buffer->cursor.y]->length > 0 && buffer->lines[buffer->cursor.y - 1]->capacity - buffer->lines[buffer->cursor.y - 1]->length > buffer->lines[buffer->cursor.y]->length){
                memcpy(buffer->lines[buffer->cursor.y - 1]->text + buffer->lines[buffer->cursor.y - 1]->length, buffer->lines[buffer->cursor.y]->text, buffer->lines[buffer->cursor.y]->length);
            }

            /* update new length */
            buffer->lines[buffer->cursor.y - 1]->length += buffer->lines[buffer->cursor.y]->length;
            
            /* Move the content of all lines below current up one line */
            if(buffer->cursor.y < buffer->line_count - 1){
                for (size_t i = buffer->cursor.y; i < buffer->line_count - 1; i++) {
                    buffer->lines[i]->text = buffer->lines[i + 1]->text;
                    buffer->lines[i]->length = buffer->lines[i + 1]->length;
                    buffer->lines[i]->capacity = buffer->lines[i + 1]->capacity;
                }
            }

            /* Remove the last line */
            kfree(buffer->lines[buffer->line_count - 1]->text);
            buffer->lines[buffer->line_count - 1]->text = NULL;
            buffer->lines[buffer->line_count - 1]->length = 0;
            buffer->lines[buffer->line_count - 1]->capacity = 0;
            buffer->line_count--;

            buffer->cursor.x = buffer->lines[buffer->cursor.y - 1]->length;
            buffer->cursor.y--;
        } else {
            /* Remove the character before the cursor */
            if(buffer->lines[buffer->cursor.y] == NULL) {
                buffer->cursor.x = 0;
                return;
            }

            if(buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
                for (size_t i = buffer->cursor.x; i < buffer->lines[buffer->cursor.y]->length; i++) {
                    buffer->lines[buffer->cursor.y]->text[i - 1] = buffer->lines[buffer->cursor.y]->text[i];
                }
                buffer->lines[buffer->cursor.y]->text[buffer->lines[buffer->cursor.y]->length - 1] = '\0';
                buffer->lines[buffer->cursor.y]->length--;
                buffer->cursor.x--; 
                return;
            }

            buffer->lines[buffer->cursor.y]->text[buffer->cursor.x - 1] = '\0';
            buffer->lines[buffer->cursor.y]->length--;
            buffer->cursor.x--;
        }
    } else if (c == '\n') {
        /* If the cursor is at the end of the line, create a new line */
        if (buffer->cursor.x == buffer->lines[buffer->cursor.y]->length) {
            /* If the buffer is full, do nothing */
            if (buffer->line_count == MAX_LINES) {
                return;
            }

            /* Create a new line */
            buffer->lines[buffer->line_count] = kalloc(sizeof(struct line));
            buffer->lines[buffer->line_count]->text = kalloc(32);
            buffer->lines[buffer->line_count]->length = 0;
            buffer->lines[buffer->line_count]->capacity = 32;
            buffer->line_count++;

            /* Move the cursor to the beginning of the new line */
            buffer->cursor.x = 0;
            buffer->cursor.y++;
        } else {
            /* Move down the content of all lines below current */
            for (size_t i = buffer->line_count - 1; i > buffer->cursor.y + 1; i--) {
                buffer->lines[i]->text = buffer->lines[i - 1]->text;
                buffer->lines[i]->length = buffer->lines[i - 1]->length;
                buffer->lines[i]->capacity = buffer->lines[i - 1]->capacity;
            }

            /* Create a new line */
            buffer->lines[buffer->cursor.y + 1] = kalloc(sizeof(struct line));
            buffer->lines[buffer->cursor.y + 1]->text = kalloc(32);
            buffer->lines[buffer->cursor.y + 1]->length = 0;
            buffer->lines[buffer->cursor.y + 1]->capacity = 32;
            buffer->line_count++;


            /* clear next line */
            memset(buffer->lines[buffer->cursor.y + 1]->text, 0, buffer->lines[buffer->cursor.y + 1]->capacity);

            /* Move the content from x to end down one line */
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
        }
    } else if(c >= ARROW_RIGHT && c <= ARROW_UP ){
        switch (c){
            case ARROW_RIGHT:
                if(buffer->cursor.x < buffer->lines[buffer->cursor.y]->length){
                    buffer->cursor.x++;
                }
                break;
            case ARROW_LEFT:
                if(buffer->cursor.x > 0){
                    buffer->cursor.x--;
                }
                break;
            case ARROW_UP:
                if(buffer->cursor.y > 0){
                    buffer->cursor.y--;
                }
                break;
            case ARROW_DOWN:
                if(buffer->cursor.y < buffer->line_count - 1){
                    buffer->cursor.y++;
                }
                break;
        }
        textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    } else {
        /* If the buffer is full, do nothing */
        if (buffer->lines[buffer->cursor.y]->length == buffer->lines[buffer->cursor.y]->capacity) {
            return;
        }

        /* If x is behind length, then we move content forward */
        if(buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
            /* Move the content from x to end forward one character */
            for (size_t i = buffer->lines[buffer->cursor.y]->length; i > buffer->cursor.x; i--) {
                buffer->lines[buffer->cursor.y]->text[i] = buffer->lines[buffer->cursor.y]->text[i - 1];
            }
        }

        /* Insert the character at the cursor position */
        buffer->lines[buffer->cursor.y]->text[buffer->cursor.x] = c;
        buffer->lines[buffer->cursor.y]->length++;
        buffer->cursor.x++;
    }
}


static __kthread_entry void editor()
{
    char c;
    struct textbuffer *buffer = textbuffer_create();

    while (1){
		c = kb_get_char();
		if(c == 0) continue;
        textbuffer_handle_char(buffer, c);
        textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	}

}
EXPORT_KSYMBOL(editor);

