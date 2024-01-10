/**
 * @file sysinf.c
 * @author Joe Bayer (joexbayer)
 * @brief System information tool.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kthreads.h>
#include <net/net.h>
#include <net/interface.h>
#include <net/netdev.h>
#include <memory.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <kutils.h>
#include <util.h>
#include <lib/icons.h>
#include <pci.h>

#include <diskdev.h>
#include <fs/fs.h>
#include <fs/fat16.h>
#include <scheduler.h>
#include <mbr.h>
#include <net/dhcp.h>
#include <lib/display.h>

#include <kernel.h>

#define WIDTH 300
#define HEIGHT 275

#define TABS 5

enum tab_type {
    TAB_MEM,
    TAB_NET,
    TAB_DISK,
    TAB_DEV,
    TAB_DISPLAY
};

static struct tab_view {
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
        [TAB_DEV] = {
            .type = TAB_DEV,
            .name = "Devices",
            .width = 50,
            .x = 12 + 150,
            .y = 12,
            .active = false,
        },
        [TAB_DISPLAY] = {
            .type = TAB_DISPLAY,
            .name = "Display",
            .width = WIDTH - 200-24,
            .x = 12 + 200,
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
    w->draw->textf(w, 30, 45+20, 0,     "Virtual:       %d MB", map->virtual_memory.total / 1024 / 1024);
    w->draw->textf(w, 30, 45+30, 0,     "Permanent:     %d MB", map->permanent.total / 1024 / 1024);

    char* kernel = "Usage";
    SECTION(w, 24, HEIGHT/2+10, WIDTH-48, HEIGHT/2-48, kernel);

    /* memory usage with rectangles based on percentage */
    int kernel_usage = (int)(((float)info.kernel.used / (float)info.kernel.total)*100);
    int virtual_usage = (int)(((float)info.virtual_memory.used / (float)info.virtual_memory.total)*100);
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
    
    /**
     * Sections: 
     * Stats
     * Services
     * Interface 
     */
    struct net_info info;
    net_get_info(&info);

    SECTION(w, 24, 48, WIDTH-48, HEIGHT/3-48, "Stats");

    gfx_put_icon32(wlan_32, WIDTH/2 + 40, 45+10);

    w->draw->textf(w, 30, 45+10, 0,     "TX:        %d", info.sent);
    w->draw->textf(w, 30, 45+20, 0,     "RX:        %d", info.recvd);
    w->draw->textf(w, 30, 45+30, 0,     "Dropped:   %d", info.dropped);

    SECTION(w, 24, HEIGHT/3+10, WIDTH-48, HEIGHT/3-48, "Services");

    w->draw->textf(w, 30, HEIGHT/3+10+10, 0,     "DHCP:          %s", dhcp_state_names[dhcp_get_state()] );
    w->draw->textf(w, 30, HEIGHT/3+10+20, 0,     "DNS:           %i", dhcp_get_dns());

    SECTION(w, 24, (HEIGHT/3+10) + (HEIGHT/3-48)+10, WIDTH-48, 100, "Interface");

    struct net_interface** interfaces = net_get_interfaces();
    for (int i = 0; i < 2; i++){ /* we only currently support 2 interfaces */
        if(interfaces[i] == NULL) continue;
        w->draw->textf(w, 30, (HEIGHT/3+10) + (HEIGHT/3-48)+10+10 + (i*40), 0,     "%s\n. Address:    %i\n. Netmask:    %i\n. Gateway:    %i", interfaces[i]->name, interfaces[i]->ip, ntohl(interfaces[i]->netmask), ntohl(interfaces[i]->gateway));
    }
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

static void sysinf_draw_dev(struct window* w, struct tab* tab)
{
    struct pci_device* devices = pci_get_devices();

    SECTION(w, 24, 48, WIDTH-48, HEIGHT-48-24, "PCI");

    /* Iterate through the devices */
    for(int i = 0; i < 25; i++) {
        if(devices[i].vendor == 0) continue;

        unsigned char* icon;
        switch (devices[i].class){
        case 0x6:
            icon = menu_16;
            break;
        case 0x2:
            icon = wlan_16;
            break;
        default:
            icon = desktop_16;
            break;
        }

        int y_device = 45 + 12 + (i * 24);
        int y_device_info = y_device + 12;

        /* Draw the device name */
        w->draw->textf(w, 35+18, y_device, 0, "%s (%s)", pci_get_class_name(&devices[i]), pci_get_vendor_name(&devices[i]));
        gfx_put_icon16(icon, 35, y_device-4);

        /* Draw additional device information */
        w->draw->textf(w, 53, y_device_info, 0, "%s", pci_get_device_name(&devices[i]));

        /* Draw tree lines */
        /* Horizontal line from icon to text */
        w->draw->line(w, y_device + 4, 30, y_device+4, 35,0);
        w->draw->line(w, y_device_info + 4, 30, y_device_info+4, 50,0);
        
        /* Check if it's not the last device to draw a vertical line */
        if (i < 24 ) {
            w->draw->line(w, y_device + 4, 30, y_device + 4 + 24, 30,0);
        }
    }



}

static void sysinf_draw_display(struct window* w, struct tab* tab)
{
    struct display_info info;
    display_get_info(&info);

    /* Display */
    SECTION(w, 24, 48, WIDTH-48, HEIGHT/3-48, "Display");

    w->draw->textf(w, 30, 45+10, 0,     "Width:    %d", info.width);
    w->draw->textf(w, 30, 45+20, 0,     "Height:   %d", info.height);
    w->draw->textf(w, 30, 45+30, 0,     "Depth:    %d", info.bpp);


    gfx_put_icon32(screen_32, WIDTH/2 + 40, 45+10);

    /* Colors */
    SECTION(w, 24, HEIGHT/3+10, WIDTH-48, HEIGHT-48, "Colors - VGA");

    /* print all colors */
    for(int i = 0; i < 16; i++){
        for (int j = 0; j < 16; j++){
            w->draw->box(w, 65 + (i*9), HEIGHT/3+10+10 + (j*9), 9, 9, j*16+i);
        }
    }
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
            sysinf_draw_display(w, tab);
            break;
        case TAB_DEV:
            sysinf_draw_dev(w, tab);
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

    /* draw the bottom text */
    w->draw->textf(w, 12, HEIGHT-10, 0, "Kernel: %s", KERNEL_VERSION);
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