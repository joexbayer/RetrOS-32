#ifndef FCA672BC_C2FD_4772_BC32_C01EF99BEA47
#define FCA672BC_C2FD_4772_BC32_C01EF99BEA47

#include <gfx/window.h>

void gfx_composition_add_window(struct window* w);
void gfx_composition_remove_window(struct window* w);

void gfx_init();

void gfx_compositor_main();
void gfx_mouse_event(int x, int y, char flags);
void gfx_set_fullscreen(struct window* w);

#endif /* FCA672BC_C2FD_4772_BC32_C01EF99BEA47 */
