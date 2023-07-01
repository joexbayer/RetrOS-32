#ifndef FA227328_E158_4ACA_9652_4D12EC113A2B
#define FA227328_E158_4ACA_9652_4D12EC113A2B

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

/* enum for mouse flags:  Y overflow 	X overflow 	Y sign bit 	X sign bit 	Always 1 	Middle Btn 	Right Btn 	Left Btn  */
typedef enum mouse_flags {
    MOUSE_LEFT = 1 << 0,
    MOUSE_RIGHT = 1 << 1,
    MOUSE_MIDDLE = 1 << 2,
    MOUSE_ALWAYS_ONE = 1 << 3,
    MOUSE_X_SIGN_BIT = 1 << 4,
    MOUSE_Y_SIGN_BIT = 1 << 5,
    MOUSE_X_OVERFLOW = 1 << 6,
    MOUSE_Y_OVERFLOW = 1 << 7,
} mouse_flag_t;


struct mouse {
    int x, y;
    char flags;
};

void mouse_init();
int mouse_event_get(struct mouse* m);

#endif /* FA227328_E158_4ACA_9652_4D12EC113A2B */
