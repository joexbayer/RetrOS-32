#ifndef FA227328_E158_4ACA_9652_4D12EC113A2B
#define FA227328_E158_4ACA_9652_4D12EC113A2B

#include <stdint.h>

/* turn defines into enum */

typedef enum ps2_mouse_commands {
    MOUSE_PORT = 0x60,
    MOUSE_STATUS = 0x64,
    MOUSE_ABIT = 0x02,
    MOUSE_BBIT = 0x01,
    MOUSE_WRITE = 0xD4,
    MOUSE_F_BIT = 0x20,
    MOUSE_V_BIT = 0x08
} ps2_mouse_command_t;

/* enum for mouse flags:  Y overflow 	X overflow 	Y sign bit 	X sign bit 	Always 1 	Middle Btn 	Right Btn 	Left Btn  */
typedef enum ps2_mouse_flags {
    MOUSE_LEFT = 1 << 0,
    MOUSE_RIGHT = 1 << 1,
    MOUSE_MIDDLE = 1 << 2,
    MOUSE_ALWAYS_ONE = 1 << 3,
    MOUSE_X_SIGN_BIT = 1 << 4,
    MOUSE_Y_SIGN_BIT = 1 << 5,
    MOUSE_X_OVERFLOW = 1 << 6,
    MOUSE_Y_OVERFLOW = 1 << 7,
} ps2_mouse_flag_t;

struct ps2_mouse_packet {
    uint8_t flags;
    int8_t x, y;
};

struct ps2_mouse {
    uint8_t cycle;
    struct ps2_mouse_packet packet;
    short x, y;
    uint8_t received;
    uint8_t initialized;
};

struct mouse {
    int x, y;
    char flags;
};

void mouse_init();
int mouse_get_event(struct mouse* m);

#endif /* FA227328_E158_4ACA_9652_4D12EC113A2B */
