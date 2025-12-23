#include "lib/std.c"
#include "lib/gfx.c"

int main(){
    int i;
    gfx_create_window(200, 120, 0);
    gfx_set_title("Hello");
    gfx_draw_rectangle(0, 0, 200, 120, 30);
    while (1==1) {
        i = i;
    } // keep window alive
    return 0;
}
