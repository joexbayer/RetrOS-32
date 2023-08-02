#include <gfx/theme.h>
#include <util.h>

struct gfx_theme default1 = {
    .os = {
        .background = 0x5b, //0x5b
        .foreground = 0x73,
        .text = 0x5b
    },
    .window = {
        .border = 0x73,
        .background = 0x5b,
        .text = 0x16
    },
    .terminal = {
        .background = COLOR_VGA_BG,
        .text = COLOR_VGA_FG
    },
    .name = "Light theme"
};

struct gfx_theme macos = {
    .os = {
        .background = COLOR_VGA_BG+1,
        .foreground = COLOR_VGA_MISC-1,
        .text = COLOR_VGA_FG
    },
    .window = {
        .border = COLOR_VGA_LIGHT_GRAY,
        .background = COLOR_VGA_LIGHTER_GRAY+1,
        .text = COLOR_VGA_MEDIUM_GRAY
    },
    .terminal = {
        .background = COLOR_VGA_BG,
        .text = COLOR_VGA_FG
    },
    .name = "MacOS theme"
};

struct gfx_theme dark_theme = {
    .os = {
        .background = COLOR_VGA_BG+1,
        .foreground = COLOR_VGA_MISC-1,
        .text = COLOR_VGA_FG
    },
    .window = {
        .border = COLOR_VGA_MISC-1,
            .background = COLOR_VGA_BG,
        .text = COLOR_VGA_FG
    },
    .terminal = {
        .background = COLOR_VGA_BG,
        .text = COLOR_VGA_FG
    },
    .name = "Dark theme"
};

struct gfx_theme temple_os_theme = {
    .os = {
        .background = 15,
        .foreground = 0x1,
        .text = 15
    },
    .window = {
        .border = 0x1,
        .background = 15,
        .text = 0x1
    },
    .terminal = {
        .background = 15,
        .text = 0x1
    },
    .name = "Temple OS"
};

#define UPDATE_MEMBER(member, new_val) (member = new_val != 0 ? new_val : member)

#define GFX_MAX_THEMES 8

static int total_themes = 4;

static struct gfx_theme* gfx_themes[GFX_MAX_THEMES] = {
    &dark_theme, &temple_os_theme,  &default1, &macos
};

static struct gfx_theme* current_theme = &macos;

int gfx_total_themes()
{
    return total_themes;
}

struct gfx_theme* kernel_gfx_current_theme()
{
    return current_theme;
}

int kernel_gfx_update_theme(struct gfx_theme theme)
{
    UPDATE_MEMBER(current_theme->os.background , theme.os.background);
    UPDATE_MEMBER(current_theme->os.foreground , theme.os.foreground);
    UPDATE_MEMBER(current_theme->os.text , theme.os.text);

    UPDATE_MEMBER(current_theme->window.background , theme.window.background);
    UPDATE_MEMBER(current_theme->window.border , theme.window.border);
    UPDATE_MEMBER(current_theme->window.text , theme.window.text);

    UPDATE_MEMBER(current_theme->terminal.background , theme.terminal.background);
    UPDATE_MEMBER(current_theme->terminal.text , theme.terminal.text);
    return 0;
}

struct gfx_theme* kernel_gfx_default_theme()
{
    return &macos;
}

int kernel_gfx_set_theme(int index)
{
    if(index >= total_themes)
        return -1;
    current_theme = gfx_themes[index];
    return 0;
}

struct gfx_theme* kernel_gfx_get_theme(int index)
{
    if(index >= total_themes)
        return NULL;
    return gfx_themes[index];
}