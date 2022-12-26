#ifndef FA227328_E158_4ACA_9652_4D12EC113A2B
#define FA227328_E158_4ACA_9652_4D12EC113A2B

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

struct mouse {
    int x, y;
    char flags;
};

void mouse_init();
int mouse_event_get(struct mouse* m);

#endif /* FA227328_E158_4ACA_9652_4D12EC113A2B */
