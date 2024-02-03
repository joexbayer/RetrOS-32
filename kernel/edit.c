#include <kernel.h>
#include <kutils.h>
#include <memory.h>
#include <screen.h>
#include <colors.h>
#include <ksyms.h>
#include <kthreads.h>
#include <keyboard.h>

#define MAX_LINES 512
#define LINE_CAPACITY 78

struct textbuffer {
    struct line {
        char *text;
        size_t length;
        size_t capacity;
    } **lines;
    struct cursor {
        size_t x;
        size_t y;
    } cursor;
    size_t line_count;
};
static int textbuffer_new_line(struct textbuffer *buffer);

static struct textbuffer *textbuffer_create(void)
{
    int ret;
    struct textbuffer *buffer = create(struct textbuffer);
    if(buffer == NULL){
        return NULL;
    }
    
    buffer->lines = kalloc(sizeof(struct line*) * MAX_LINES);
    if(buffer->lines == NULL){
        kfree(buffer);
        return NULL;
    }

    ret = textbuffer_new_line(buffer);
    if(ret < 0){
        kfree(buffer->lines);
        kfree(buffer);
        return NULL;
    }

    buffer->cursor.x = 0;
    buffer->cursor.y = 0;

    return buffer;
}

static void textbuffer_destroy(struct textbuffer *buffer)
{
    for (size_t i = 0; i < buffer->line_count; i++) {
        if (buffer->lines[i]->text) {
            kfree(buffer->lines[i]->text);
        }
        kfree(buffer->lines[i]);
    }
    kfree(buffer->lines);
    kfree(buffer);
}

static int textbuffer_new_line(struct textbuffer *buffer)
{
    if (buffer->line_count == MAX_LINES) {
        return -1;
    }

    buffer->lines[buffer->line_count] = kalloc(sizeof(struct line));
    if(buffer->lines[buffer->line_count] == NULL){
        return -1;
    }

    buffer->lines[buffer->line_count]->text = kalloc(LINE_CAPACITY);
    if(buffer->lines[buffer->line_count]->text == NULL){
        kfree(buffer->lines[buffer->line_count]);
        return -1;
    }

    memset(buffer->lines[buffer->line_count]->text, 0, LINE_CAPACITY);
    
    buffer->lines[buffer->line_count]->length = 0;
    buffer->lines[buffer->line_count]->capacity = LINE_CAPACITY;
    buffer->line_count++;

    return 0;
}

static int textbuffer_remove_last_line(struct textbuffer *buffer)
{
    if (buffer->line_count == 0) {
        return -1;
    }

    kfree(buffer->lines[buffer->line_count - 1]->text);
    kfree(buffer->lines[buffer->line_count - 1]);
    buffer->line_count--;

    return 0;
}

static int textbuffer_free_line(struct textbuffer *buffer, size_t line)
{
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
    kfree(buffer->lines[buffer->line_count - 1]->text);
    kfree(buffer->lines[buffer->line_count - 1]);
    buffer->line_count--;

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
            scrprintf(x, 1+y, "%d: %s (l: %d)", i, buffer->lines[i]->text, buffer->lines[i]->length);
        }
    }

    /* Set the cursor to the end of the text */
    if (buffer->line_count > 0) {
        screen_set_cursor(x_start, y_start + buffer->line_count - 1);
    }

    screen_set_cursor(buffer->cursor.x-1+3, 1+buffer->cursor.y);
    /* write stats at the bottom */
    scrprintf(0, 24, "line_count: %d, x: %d, y: %d", buffer->line_count, buffer->cursor.x, buffer->cursor.y);
}

/* Function to handle keyboard input of a char */
static void textbuffer_handle_char(struct textbuffer *buffer, unsigned char c) {
    int ret;
    
    /* Handle the backspace key */
    if (c == '\b') {
        /* If we are at the end of the line */
        if (buffer->cursor.x == 0) {
            /* If the cursor is at the beginning of the first line, do nothing */
            if (buffer->cursor.y == 0 ) {
                return;
            }

            /* Move the rest of the current line up if there is space. */
            if(buffer->lines[buffer->cursor.y]->length > 0 && buffer->lines[buffer->cursor.y - 1]->capacity - buffer->lines[buffer->cursor.y - 1]->length > buffer->lines[buffer->cursor.y]->length){
                memcpy(buffer->lines[buffer->cursor.y - 1]->text + buffer->lines[buffer->cursor.y - 1]->length, buffer->lines[buffer->cursor.y]->text, buffer->lines[buffer->cursor.y]->length);
            }
            buffer->cursor.x = buffer->lines[buffer->cursor.y - 1]->length;

            /* update new length */
            buffer->lines[buffer->cursor.y - 1]->length += buffer->lines[buffer->cursor.y]->length;
            
            textbuffer_free_line(buffer, buffer->cursor.y);

            buffer->cursor.y--;
        } else {
            /* Remove the character before the cursor */
            if(buffer->lines[buffer->cursor.y] == NULL) {
                buffer->cursor.x = 0;
                dbgprintf("buffer->lines[buffer->cursor.y] == NULL %d, 0x%x\n", buffer->cursor.y, buffer->lines[buffer->cursor.y]);
                return;
            }

            /* If x is behind length, then we move content back */
            if(buffer->cursor.x < buffer->lines[buffer->cursor.y]->length) {
                for (size_t i = buffer->cursor.x; i < buffer->lines[buffer->cursor.y]->length; i++) {
                    buffer->lines[buffer->cursor.y]->text[i - 1] = buffer->lines[buffer->cursor.y]->text[i];
                }
                buffer->lines[buffer->cursor.y]->text[buffer->lines[buffer->cursor.y]->length - 1] = '\0';
                buffer->lines[buffer->cursor.y]->length--;
                buffer->cursor.x--; 
                return;
            }

            /* If x is at length, then we just remove the last character */
            buffer->lines[buffer->cursor.y]->text[buffer->cursor.x - 1] = 0;
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

            ret = textbuffer_new_line(buffer);
            if(ret < 0){
                warningf("textbuffer_new_line failed\n");
                return;
            }

            if(buffer->cursor.y != buffer->line_count - 1){
                /* Move down the content of all lines below current */
                for (size_t i = buffer->line_count - 1; i > buffer->cursor.y + 1; i--) {
                    memset(buffer->lines[i]->text, 0, buffer->lines[i]->capacity);
                    memcpy(buffer->lines[i]->text, buffer->lines[i - 1]->text, buffer->lines[i - 1]->length);
                    buffer->lines[i]->length = buffer->lines[i - 1]->length;
                }
                /* clear next line */
                memset(buffer->lines[buffer->cursor.y + 1]->text, 0, buffer->lines[buffer->cursor.y + 1]->capacity);
                buffer->lines[buffer->cursor.y + 1]->length = 0;
            }

            /* Move the cursor to the beginning of the new line */
            buffer->cursor.x = 0;
            buffer->cursor.y++;
        } else {
            /* Create a new line */
            ret = textbuffer_new_line(buffer);
            if(ret < 0){
                warningf("textbuffer_new_line failed\n");
                return;
            }

            /* Move down the content of all lines below current */
            for (size_t i = buffer->line_count - 1; i > buffer->cursor.y + 1; i--) {
                memcpy(buffer->lines[i]->text, buffer->lines[i - 1]->text, buffer->lines[i - 1]->length);
                buffer->lines[i]->length = buffer->lines[i - 1]->length;
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
                    if(buffer->cursor.x > buffer->lines[buffer->cursor.y]->length){
                        buffer->cursor.x = buffer->lines[buffer->cursor.y]->length;
                    }
                }
                break;
            case ARROW_DOWN:
                if(buffer->cursor.y < buffer->line_count - 1){
                    buffer->cursor.y++;
                    if(buffer->cursor.x > buffer->lines[buffer->cursor.y]->length){
                        buffer->cursor.x = buffer->lines[buffer->cursor.y]->length;
                    }
                }
                break;
        }
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
    unsigned char c;
    struct textbuffer *buffer = textbuffer_create();
    if(buffer == NULL){
        dbgprintf("buffer == NULL\n");
    }
    
    textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    while (1){
		c = kb_get_char();
		if(c == 0) continue;
        if(c == CTRLC) break;

        textbuffer_handle_char(buffer, c);
        textbuffer_display(buffer, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	}

    textbuffer_destroy(buffer);
}
EXPORT_KSYMBOL(editor);

