#ifndef B176ABF6_C7E4_4181_ACF2_0C0D91825623
#define B176ABF6_C7E4_4181_ACF2_0C0D91825623

#include <stdint.h>
#include <colors.h>

/* Struct for graphic themes */
struct gfx_theme {
    struct {
        color_t background;
        color_t foreground;
        color_t text;
    } os;

    struct {
        color_t border;
        color_t background;
        color_t text;
    } window;

    struct {
        color_t text;
        color_t background;
    } border;

    int test;
};

struct gfx_theme* kernel_gfx_current_theme();
int kernel_gfx_set_theme(int index);
struct gfx_theme* kernel_gfx_get_theme(int index);

#endif /* B176ABF6_C7E4_4181_ACF2_0C0D91825623 */
