#include <kthreads.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <kutils.h>
#include <util.h>

#define WIDTH 250
#define HEIGHT 275

#define TABS 4

enum tab_type {
    TAB_MEM,
    TAB_NET,
    TAB_DISK,
    TAB_DISPLAY,
};

struct tab_view {
    int width, height;
    int x, y;

    int tab_width;

    struct tab {
        enum tab_type type;
        char* name;
        int width;
        int x, y;
        bool_t active;
    } tabs[TABS];

} tab_view = {
    .width = WIDTH,
    .height = HEIGHT,
    .x = 0,
    .y = 0,
    .tab_width = 30,
    .tabs = {
        [TAB_MEM] = {
            .type = TAB_MEM,
            .name = "Mem",
            .width = 50,
            .x = 12,
            .y = 12,
            .active = true,
        },
        [TAB_NET] = {
            .type = TAB_NET,
            .name = "Net",
            .width = 50,
            .x = 12 + 50,
            .y = 12,
            .active = false,
        },
        [TAB_DISK] = {
            .type = TAB_DISK,
            .name = "Disk",
            .width = 50,
            .x = 12 + 100,
            .y = 12,
            .active = false,
        },
        [TAB_DISPLAY] = {
            .type = TAB_DISPLAY,
            .name = "Display",
            .width = WIDTH - 150-24,
            .x = 12 + 150,
            .y = 12,
            .active = false,
        },
    }
};

static void sysinf_draw_tab(struct window* w, struct tab* tab)
{
    if(!tab->active){
        gfx_draw_contoured_box(tab->x, tab->y, tab->width, 20, 30);
    }
    w->draw->text(w, tab->x + 5, tab->y + 5, tab->name, 0);
}

static void sysinf_draw(struct window* w)
{
    struct gfx_theme* theme = kernel_gfx_current_theme();

    w->draw->rect(w, 0, 0, WIDTH, HEIGHT, 30);

    gfx_draw_contoured_box(12, 12, WIDTH-24, HEIGHT-24, 30);

    for(int i = 0; i < 5; i++){
        struct tab* tab = &tab_view.tabs[i];
        sysinf_draw_tab(w, tab);
    }
}

static void sysinf_click_event(struct window* w, int x, int y)
{
    for(int i = 0; i < TABS; i++){
        struct tab* tab = &tab_view.tabs[i];
        if(x >= tab->x && x <= tab->x + tab->width && y >= tab->y && y <= tab->y + 20){
            tab_view.tabs[i].type = i;
            sysinf_draw_tab(w, tab);
        }
    }
}

void __kthread_entry sysinf(int argc, char* argv[])
{   
    struct gfx_theme* theme;
    
    struct window* w = gfx_new_window(WIDTH, HEIGHT, 0);
    if(w == NULL){
        warningf("Failed to create window for sysinf");
        return;
    }

    w->ops->move(w, 50, 50);
    while (1){
        theme = kernel_gfx_current_theme();

        sysinf_draw(w);

        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;
        switch (event.event){
        case GFX_EVENT_EXIT:
            kernel_exit();
            break;
        case GFX_EVENT_KEYBOARD:
            /* key in event.data */
            break;
        case GFX_EVENT_MOUSE:{
                sysinf_click_event(w, event.data, event.data2);
            }
            break;
        default:
            break;
        }

    }
}
EXPORT_KTHREAD(sysinf);