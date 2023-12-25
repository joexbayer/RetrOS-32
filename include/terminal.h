#ifndef TERMINAL_H
#define TERMINAL_H

#include <kutils.h>
#include <colors.h>
#include <gfx/window.h>
#include <util.h>

#define twritef(a, ...) \
 if(current_running != NULL && current_running->term != NULL) { \
    current_running->term->ops->writef(current_running->term, a, ##__VA_ARGS__); \
 }

typedef enum {
    TERMINAL_FLAG_NONE = 0 << 0,
    TERMINAL_TEXT_MODE = 1 << 0,
    TERMINAL_GRAPHICS_MODE = 1 << 1,
} terminal_flags_t;

struct terminal;
struct terminal_ops {
    int (*write)(struct terminal* term, const char* data, int size);
    int (*writef)(struct terminal* term, char* fmt, ...);
    int (*putchar)(struct terminal* term, char c);
    int (*commit)(struct terminal* term);
    int (*attach)(struct terminal* term);
    int (*set)(struct terminal* term, struct terminal_ops* ops);
    int (*detach)(struct terminal* term);
    int (*reset)(struct terminal* term);

    /**
     * @brief Read data from the terminal (keyboard)
     * @param term Terminal to read from
     * @param data Buffer to read into
     * @param size Size of the buffer
     * 
     * @return int Amount of bytes read
     */
    int (*scan)(struct terminal* term, ubyte_t* data, int size);
    int (*scanf)(struct terminal* term, char* fmt, ...);
};
struct terminal {
    char* textbuffer;

    int tail;
    int head;
    int lines;

    struct kref ref;

    struct window* screen;
    struct terminal_ops* ops;
    color_t text_color;
    color_t bg_color;
};

struct terminal* terminal_create(terminal_flags_t flags);
int terminal_destroy(struct terminal* term);

int scan(ubyte_t* data, int size);

void twrite(const char* data);
void terminal_write(const char* data, int size);
void terminal_putchar(char c);


void terminal_commit();
void terminal_attach(struct terminal* term);
void twriteln(const char* data);

#endif