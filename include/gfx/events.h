#ifndef D05A48F1_86E3_4F50_8DF8_56BA8BEC696B
#define D05A48F1_86E3_4F50_8DF8_56BA8BEC696B

#include <stdint.h>
#include <keyboard.h>

#define KEY_UP 254
#define KEY_DOWN 253
#define KEY_LEFT 252
#define KEY_RIGHT 251
#define KEY_F3 248
#define KEY_F1 250

enum gfx_events {
    GFX_EVENT_KEYBOARD,
    GFX_EVENT_MOUSE,
    GFX_EVENT_EXIT
};

struct gfx_event {
    uint8_t event;
    uint16_t data;
    uint16_t data2;
};

#endif /* D05A48F1_86E3_4F50_8DF8_56BA8BEC696B */
