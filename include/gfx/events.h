#ifndef D05A48F1_86E3_4F50_8DF8_56BA8BEC696B
#define D05A48F1_86E3_4F50_8DF8_56BA8BEC696B

#include <stdint.h>

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
