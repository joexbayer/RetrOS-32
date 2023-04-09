#include <gfx/theme.h>

struct gfx_theme default1 = {
    .border = {
        .background = COLOR_BOX_LIGHT_FG,
        .text = COLOR_BOX_LIGHT_FG
    },
    .os = {
        .background = COLOR_BOX_BG,
        .foreground = COLOR_BOX_LIGHT_FG,
        .text = COLOR_BOX_BG
    },
    .window = {
        .border = COLOR_BOX_LIGHT_BLUE,
        .background = COLOR_BOX_BG,
        .text = COLOR_BOX_BG
    }
};

struct gfx_theme default2 = {
    .border = {
        .background = COLOR_WHITE,
        .text = COLOR_LIGHT_YELLOw
    },
    .os = {
        .background = COLOR_BLACK,
        .foreground = COLOR_WHITE,
        .text = COLOR_YELLOW
    },
    .window = {
        .border = COLOR_BLUE,
        .background = COLOR_BLACK,
        .text = COLOR_WHITE
    }
};

struct gfx_theme* gfx_themes[] = {
    &default1, &default2
};

static struct gfx_theme* current_theme = &default1;

struct gfx_theme* kernel_gfx_current_theme()
{
    return current_theme;
}

int kernel_gfx_set_theme(int index)
{
    current_theme = gfx_themes[index];
    return 0;
}

struct gfx_theme* kernel_gfx_get_theme(int index)
{
    /* TODO: if check */
    return gfx_themes[index];
}