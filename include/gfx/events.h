#ifndef D05A48F1_86E3_4F50_8DF8_56BA8BEC696B
#define D05A48F1_86E3_4F50_8DF8_56BA8BEC696B

#include <stdint.h>
#include <keyboard.h>

#define KEY_UP 254
#define KEY_DOWN 253
#define KEY_LEFT 252
#define KEY_RIGHT 251
#define KEY_F9 242 /* CTRL r */
#define KEY_F4 247
#define KEY_F3 248
#define KEY_F2 249
#define KEY_F1 250

typedef enum gfx_event_flags {
    GFX_EVENT_BLOCKING = 1 << 0,
    GFX_EVENT_NONBLOCKING = 1 << 1
} gfx_event_flag_t;

typedef enum gfx_events {
    GFX_EVENT_KEYBOARD,
    GFX_EVENT_MOUSE,
    GFX_EVENT_EXIT,
    GFX_EVENT_RESOLUTION
} gfx_event_t;

struct gfx_event {
    uint8_t event;
    uint16_t data;
    uint16_t data2;
};

struct gfx_event_key {
    uint8_t event;
    uint16_t data;
    uint16_t unused;
};

struct gfx_event_mouse {
    uint8_t event;
    uint16_t x;
    uint16_t y;
};

#endif /* D05A48F1_86E3_4F50_8DF8_56BA8BEC696B */
