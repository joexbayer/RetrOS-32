#include <kthreads.h>
#include <memory.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <kutils.h>
#include <util.h>
#include <lib/icons.h>

#include <diskdev.h>
#include <fs/fs.h>
#include <mbr.h>

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

#define SECTION(w, x, y, width, height, header)\
    w->draw->box(w, x, y, width, height, 30);\
    w->draw->rect(w, x+6, y-3, strlen(header)*8, 8, 30);\
    w->draw->text(w, x+6, y-3, header, 0);

static void sysinf_draw_mem(struct window* w, struct tab* tab)
{
    struct mem_info info;
    struct memory_map* map = memory_map_get();
    get_mem_info(&info);

    w->draw->text(w, 24, 48, "Kernel", 0);

    /* Memory map segments */
    char* header = "Map";
    SECTION(w, 24, 48, WIDTH-48, HEIGHT/2-48, header);

    w->draw->textf(w, 30, 45+40, 0,     "Total:        %d MB", map->total / 1024 / 1024);
    w->draw->textf(w, 30, 45+10, 0,     "Kernel:        %d MB", map->kernel.total / 1024 / 1024);
    w->draw->textf(w, 30, 45+20, 0,     "Virtual:       %d MB", map->virtual.total / 1024 / 1024);
    w->draw->textf(w, 30, 45+30, 0,     "Permanent:     %d MB", map->permanent.total / 1024 / 1024);

    char* kernel = "Usage";
    SECTION(w, 24, HEIGHT/2+10, WIDTH-48, HEIGHT/2-48, kernel);

    /* memory usage with rectangles based on percentage */
    int kernel_usage = (int)(((float)info.kernel.used / (float)info.kernel.total)*100);
    int virtual_usage = (int)(((float)info.virtual.used / (float)info.virtual.total)*100);
    int permanent_usage = (int)(((float)info.permanent.used / (float)info.permanent.total)*100);

    w->draw->box(w, 30, HEIGHT/2+10+10, 100, 10, 0);
    w->draw->rect(w, 31, HEIGHT/2+10+11, kernel_usage, 8, COLOR_VGA_GREEN);
    w->draw->textf(w, 30, HEIGHT/2+10+10+12, 0, "Kernel: %d%", kernel_usage);

    w->draw->box(w, 30, HEIGHT/2+10+10+10+10, 100, 10, 0);
    w->draw->rect(w, 31, HEIGHT/2+10+10+10+11, virtual_usage, 8, COLOR_VGA_GREEN);
    w->draw->textf(w, 30, HEIGHT/2+10+10+10+10+12, 0, "Virtual: %d%", virtual_usage);

    w->draw->rect(w, 30, HEIGHT/2+10+10+10+10+10+10, 100, 10, 0);
    w->draw->rect(w, 31, HEIGHT/2+10+10+10+10+10+11, permanent_usage, 8, COLOR_VGA_GREEN);
    w->draw->textf(w, 30, HEIGHT/2+10+10+10+10+10+10+12, 0, "Permanent: %d%", permanent_usage);

    gfx_put_icon32(computer_icon, 175, HEIGHT/2+10+10+10+5);
}

static void sysinf_draw_net(struct window* w, struct tab* tab)
{

}

static void sysinf_draw_disk(struct window* w, struct tab* tab)
{
    /* Disk */
    /* MBR */
    /* Partitions */
    /* Filesystems */

    if(!disk_attached()){
        w->draw->text(w, 24, 48, "No disk attached", 0);
        return;
    }

    SECTION(w, 24, 48, WIDTH-48, HEIGHT/3-48, "Disk");

    w->draw->textf(w, 30, 45+10, 0,     "Name:    %s", disk_name());
    w->draw->textf(w, 30, 45+20, 0,     "Size:    %d MB", disk_size() / 1024 / 1024);


    SECTION(w, 24, HEIGHT/3+10, WIDTH-48, HEIGHT/3-48, "MBR");

    struct mbr* mbr = mbr_get();
    for(int i = 0; i < 4; i++){
        if(mbr->part[i].type == 0) continue;        
        w->draw->textf(w, 30, HEIGHT/3+10+10, 0,     "%s", mbr_partition_type_string(mbr->part[i].type));
        w->draw->textf(w, 30, HEIGHT/3+10+20, 0,     "LBA %d to %d", mbr->part[i].lba_start, mbr->part[i].lba_start + mbr->part[i].num_sectors);
    }

    SECTION(w, 24, (HEIGHT/3+10) + (HEIGHT/3-48)+10, WIDTH-48, 100, "Filesystem");

    int usage = (int)(((float)fat16_used_blocks() / (float)65536)*100);

    struct filesystem* fs = fs_get();

    w->draw->textf(w, 30, (HEIGHT/3+10) + (HEIGHT/3-48)+10+10, 0, "Name:    %s", fs->name);
    w->draw->textf(w, 30, (HEIGHT/3+10) + (HEIGHT/3-48)+10+20, 0, "Usage:   %d/%d blocks", fat16_used_blocks(), 65536);

    w->draw->box(w, 30, (HEIGHT/3+10) + (HEIGHT/3-48)+10+30, 100, 10, 0);
    w->draw->rect(w, 31, (HEIGHT/3+10) + (HEIGHT/3-48)+10+31, usage, 8, COLOR_VGA_GREEN);
    w->draw->textf(w, 30+109, (HEIGHT/3+10) + (HEIGHT/3-48)+10+31, 0, "%d%", usage);

}


static void sysinf_draw_tab(struct window* w, struct tab* tab)
{   
    if(!tab->active){
        w->draw->box(w, tab->x, tab->y, tab->width, 20, 30);
    }
    w->draw->text(w, tab->x + 5, tab->y + 5, tab->name, 0);
    
    if(!tab->active) return;
    switch(tab->type){
        case TAB_MEM:
            sysinf_draw_mem(w, tab);
            break;
        case TAB_NET:
            sysinf_draw_net(w, tab);
            break;
        case TAB_DISK:
            sysinf_draw_disk(w, tab);
            break;
        case TAB_DISPLAY:
            break;
    }
}

static void sysinf_draw(struct window* w)
{
    w->draw->rect(w, 0, 0, WIDTH, HEIGHT, 30);
    w->draw->box(w, 12, 12, WIDTH-24, HEIGHT-24, 30);

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
            tab_view.tabs[i].active = true;

            /* deactivate others */
            for(int j = 0; j < TABS; j++){
                if(j == i) continue;
                tab_view.tabs[j].active = false;
            }
            return;
        } 
    }
}

void __kthread_entry sysinf(int argc, char* argv[])
{   
    struct window* w = gfx_new_window(WIDTH, HEIGHT, 0);
    if(w == NULL){
        warningf("Failed to create window for sysinf");
        return;
    }

    /* set title */
    kernel_gfx_set_title("System Information");

    w->ops->move(w, 50, 50);
    while (1){

        sysinf_draw(w);
        gfx_commit();

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