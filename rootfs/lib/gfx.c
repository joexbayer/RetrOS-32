// rootfs/lib/gfx.c
enum {
    SYSCALL_GFX_WINDOW = 5,
    SYSCALL_GFX_DRAW = 8,
    SYSCALL_GFX_SET_TITLE = 9
};

enum {
    GFX_DRAW_RECTANGLE_OPT = 3
};

enum {
    GFX_RGB = 0,
    GFX_VGA = 1
};

struct gfx_rectangle {
    int x, y, width, height;
    char color;
    char palette;
};

int gfx_draw_syscall(int option, void* data, int flags){
    return __interrupt(0x30, SYSCALL_GFX_DRAW, option, data, flags, 0);
}

void gfx_create_window(int w, int h, int flags){
    __interrupt(0x30, SYSCALL_GFX_WINDOW, w, h, flags, 0);
}

int gfx_set_title(char* title){
    return __interrupt(0x30, SYSCALL_GFX_SET_TITLE, title, 0, 0, 0);
}

int gfx_draw_rectangle(int x, int y, int w, int h, char color){
    struct gfx_rectangle r;
    r.x = x; r.y = y; r.width = w; r.height = h;
    r.color = color; r.palette = GFX_VGA;
    return gfx_draw_syscall(GFX_DRAW_RECTANGLE_OPT, &r, 0);
}
