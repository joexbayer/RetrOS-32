#include <gfx/theme.h>

struct gfx_theme default1 = {
    .os = {
        .background = COLOR_BOX_BG,
        .foreground = COLOR_BOX_DARK_BLUE,
        .text = COLOR_BOX_BG
    },
    .window = {
        .border = COLOR_BOX_DARK_BLUE,
        .background = COLOR_BOX_BG,
        .text = COLOR_BOX_BG
    },
    .terminal = {
        .background = COLOR_BOX_BG,
        .text = COLOR_BOX_DARK_BLUE
    },
    .name = "Default"
};

struct gfx_theme dark_theme = {
    .os = {
        .background = COLOR_VGA_BG,
        .foreground = COLOR_VGA_MISC,
        .text = COLOR_VGA_FG
    },
    .window = {
        .border = COLOR_VGA_MISC,
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
        .background = COLOR_WHITE,
        .foreground = 0x3,
        .text = COLOR_WHITE
    },
    .window = {
        .border = 0x3,
        .background = COLOR_WHITE,
        .text = 0x3
    },
    .terminal = {
        .background = COLOR_WHITE,
        .text = 0x3
    },
    .name = "Temple OS"
};

#define UPDATE_MEMBER(member, new_val) (member = new_val != 0 ? new_val : member)

#define GFX_MAX_THEMES 8

static int total_themes = 3;

static struct gfx_theme* gfx_themes[GFX_MAX_THEMES] = {
    &dark_theme, &temple_os_theme,  &default1,
};

static struct gfx_theme* current_theme = &dark_theme;

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
    return &default1;
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
        return -1;
    return gfx_themes[index];
}