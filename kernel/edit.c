#include <kernel.h>
#include <kutils.h>
#include <memory.h>

#define MAX_LINES 512

struct line {
    char *text;
    size_t length;
    size_t capacity;
};

struct textbuffer {
    struct line **lines;
    size_t line_count;
};

static struct textbuffer *textbuffer_create(void)
{
    struct textbuffer *buffer = create(struct textbuffer);
    buffer->lines = malloc(sizeof(struct line*) * MAX_LINES);
    buffer->line_count = 0;
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
