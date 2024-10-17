/**
 * @file taskbar.c
 * @author Joe Bayer (joexbayer)
 * @brief Taskbar for the GUI
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kthreads.h>
#include <pcb.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <vbe.h>
#include <colors.h>
#include <rtc.h>
#include <timer.h>
#include <gfx/component.h>
#include <kutils.h>
#include <kthreads.h>
#include <gfx/composition.h>
#include <lib/icons.h>
#include <scheduler.h>
#include <msgbox.h>

#define TASKBAR_HEIGHT 20

#define TASKBAR_MAX_OPTIONS 15
#define TASKBAR_OPTIONS_HEIGHT 16
#define TASKBAR_OPTIONS_HEIGHT_TOTAL TASKBAR_OPTIONS_HEIGHT * TASKBAR_MAX_OPTIONS
#define TASKBAR_MAX_HEADERS 5

#define TASKBAR_EXT_OPT_WIDTH 100
#define TASKBAR_EXT_OPT_HEIGHT 170

#define TIME_PREFIX(unit) unit < 10 ? "0" : ""

/* prototypes to callbacks */
static void __callback taskbar_terminal();
static void __callback taskbar_editor();
static void __callback taskbar_finder();
static void __callback taskbar_cube();
static void __callback taskbar_colors();
static void __callback taskbar_clock();
static void __callback taskbar_wolfstein();
static void __callback taskbar_bg_lotr();
static void __callback taskbar_bg_lotr2();
static void __callback taskbar_bg_default();
static void __callback taskbar_bg_retro();
static void __callback taskbar_bg_calc();
static void __callback taskbar_users();
static void __callback taskbar_bg_default_color();
static void __callback taskbar_bg_default_color_gray();
static void __callback taskbar_sysinf();
static void __callback taskbar_about();
static void __callback taskbar_readme();
static void __callback taskbar_reboot();
static void __callback taskbar_shutdown();
static void __callback taskbar_workspaces();

/* prototype to taskbar thread */
static void __kthread_entry taskbar(void);

/* taskbar options */
struct taskbar_option {
    char name[50];
    unsigned char* icon;
    void (*callback)(void);
};

/* taskbar header */
struct taskbar_header {
    char name[50];
    unsigned char* icon;
    int x, y, w, h;
    bool_t extended;
    struct taskbar_option options[TASKBAR_MAX_OPTIONS];
};

/* taskbar options config */
struct taskbar_options {
    struct taskbar_header headers[TASKBAR_MAX_HEADERS];
} default_taskbar = {
    .headers = {
        {
            .x = 4,
            .y = 2,
            .w = 50,
            .h = 16,
            .name = "Start",
            .icon = menu_16,
            .options = {
                {
                    .icon = terminal_16,
                    .name = "Terminal",
                    .callback = &taskbar_terminal
                },
                {
                    .icon = finder_16,
                    .name = "Finder",
                    .callback = &taskbar_finder
                },
                {
                    .icon = desktop_16,
                    .name = "SysInfo",
                    .callback = &taskbar_sysinf
                },
                {
                    .icon = editor_16,
                    .name = "Editor",
                    .callback = &taskbar_editor
                },
                {
                    .icon = bin_16,
                    .name = "Cube",
                    .callback = &taskbar_cube
                },
                {
                    .icon = bin_16,
                    .name = "Colors",
                    .callback = &taskbar_colors
                },
                {
                    .icon = clock_16,
                    .name = "Clock",
                    .callback = &taskbar_clock
                },
                {
                    .icon = calc_16,
                    .name = "Calc",
                    .callback = &taskbar_bg_calc
                },
                {
                    .icon = bin_16,
                    .name = "Users",
                    .callback = &taskbar_users
                },
                {
                    .icon = bin_16,
                    .name = "Wolfstein",
                    .callback = &taskbar_wolfstein
                }
            
            }
        },
        {
            .x = 60,
            .y = 2,
            .w = 60,
            .h = 16,
            .icon = NULL,
            .name = "RetrOS",
            .options = {
                {
                    .icon = desktop_16,
                    .name = "Reboot",
                    .callback = &taskbar_reboot
                },
                {
                    .icon = desktop_16,
                    .name = "Shutdown",
                    .callback = &taskbar_shutdown
                },
            }
        },
        // {
        //     .x = 120,
        //     .y = 2,
        //     .w = 100,
        //     .h = 16,
        //     .icon = bin_16,
        //     .name = "Wallpaper",
        //     .options = {
        //         {
        //             .icon = bin_16,
        //             .name = "Turquoise",
        //             .callback = &taskbar_bg_default_color
        //         },
        //         {
        //             .icon = bin_16,
        //             .name = "Gray",
        //             .callback = &taskbar_bg_default_color_gray
        //         },
        //         {
        //             .icon = bin_16,
        //             .name = "LOTR 1",
        //             .callback = &taskbar_bg_lotr
        //         },
        //         {
        //             .icon = bin_16,
        //             .name = "LOTR 2",
        //             .callback = &taskbar_bg_lotr2
        //         },
        //         {
        //             .icon = bin_16,
        //             .name = "Retro",
        //            .callback = taskbar_bg_retro
        //         },
        //     }
        // },
        {
            .x = 120,
            .y = 2,
            .w = 60,
            .h = 16,
            .icon = NULL,
            .name = "Help",
            .options = {
                {
                    .icon = desktop_16,
                    .name = "About",
                    .callback = &taskbar_about
                },
                {
                    .icon = desktop_16,
                    .name = "Readme",
                    .callback = &taskbar_readme
                },
                {
                    .icon = desktop_16,
                    .name = "Workspace",
                    .callback = &taskbar_workspaces
                },

                {
                    .icon = desktop_16,
                    .name = "Reboot",
                    .callback = &taskbar_reboot
                },
                {
                    .icon = desktop_16,
                    .name = "Shutdown",
                    .callback = &taskbar_shutdown
                }, 
            }
        },
    }
};

/**
 * @brief taskbar_hdr_event handles mouse events for a header
 * Draws the extended options if the mouse has clicked on the header
 * @param w window
 * @param header header
 * @param x x position of mouse
 * @param y y position of mouse
 */
static void taskbar_hdr_event(struct window* w, struct taskbar_header* header, int x, int y)
{
    /* check if mouse event is inside a header */
    if(gfx_point_in_rectangle(header->x,header->y, header->x + header->w, header->y + header->h, x, y)){
        /* draw header */
        dbgprintf("Clicked header %s\n", header->name);

        w->draw->rect(w, header->x, header->y+20, TASKBAR_EXT_OPT_WIDTH, TASKBAR_EXT_OPT_HEIGHT, 30);

        w->draw->rect(w, header->x, header->y+20, TASKBAR_EXT_OPT_WIDTH, 1, COLOR_VGA_LIGHTER_GRAY+1);
        /* bottom */
        w->draw->rect(w, header->x, header->y+21+TASKBAR_EXT_OPT_HEIGHT-2, TASKBAR_EXT_OPT_WIDTH, 1, COLOR_VGA_DARK_GRAY);
        w->draw->rect(w, header->x, header->y+20+TASKBAR_EXT_OPT_HEIGHT-2, TASKBAR_EXT_OPT_WIDTH, 1, COLOR_VGA_DARK_GRAY+8);

        w->draw->rect(w, header->x, header->y+20, 1, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_DARK_GRAY);
        w->draw->rect(w, header->x+1, header->y+20, 1, TASKBAR_EXT_OPT_HEIGHT-1, COLOR_VGA_LIGHTER_GRAY+1);

        w->draw->rect(w, header->x+TASKBAR_EXT_OPT_WIDTH-1, header->y+20, 1, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_DARK_GRAY);
        w->draw->rect(w, header->x+TASKBAR_EXT_OPT_WIDTH-2, header->y+20, 1, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_DARK_GRAY+8);
        
        header->extended = true;

        /* draw options */
        for (int j = 0; j < TASKBAR_MAX_OPTIONS; j++){
            if(header->options[j].name[0] == 0) break;

            w->draw->box(w, header->x+4, header->y+20 + (j*TASKBAR_OPTIONS_HEIGHT) + 4, TASKBAR_EXT_OPT_WIDTH-8, 16, 30);

            if(header->options[j].icon != NULL){
                gfx_put_icon16(header->options[j].icon, header->x+4, header->y+20 + (j*TASKBAR_OPTIONS_HEIGHT) + 4);
            }
            w->draw->text(w, header->x+24, header->y+20 + (j*TASKBAR_OPTIONS_HEIGHT) + 8, header->options[j].name, COLOR_BLACK);
        }
    }
}

/**
 * @brief taskbar_hdr_opt_event handles mouse events for a header's options
 * @param w window
 * @param header header
 * @param x x position of mouse
 * @param y y position of mouse
 */
static void taskbar_hdr_opt_event(struct window* w, struct taskbar_header* header, int x, int y)
{
    for (int j = 0; j < TASKBAR_MAX_OPTIONS; j++){
        if(header->options[j].name[0] == '\0') break;
        
        if(gfx_point_in_rectangle(
                header->x+4, /* x, 4 padding */
                header->y+20 + (j*TASKBAR_OPTIONS_HEIGHT) + 4, /* y, 18 offset from header */
                header->x+4 + TASKBAR_EXT_OPT_WIDTH, /* width */
                header->y+20 + (j*TASKBAR_OPTIONS_HEIGHT) + 4 + 16, /* height */
                x, y) /* mouse position */
            ){
            dbgprintf("Clicked option %s\n", header->options[j].name);

            
            if(header->options[j].callback != NULL){
                header->options[j].callback();
            }
            return;
        }
    }
}

/**
 * @brief taskbar is the main taskbar thread
 */
static void __kthread_entry taskbar(void)
{
    struct window* w = gfx_new_window(vbe_info->width, 200, GFX_IS_IMMUATABLE | GFX_HIDE_HEADER | GFX_HIDE_BORDER | GFX_IS_TRANSPARENT);
    if(w == NULL){
        warningf("Failed to create window for taskbar");
        return;
    }

    w->ops->move(w, 0, 0);
    w->draw->rect(w, 0, TASKBAR_HEIGHT+1, vbe_info->width, 1, COLOR_VGA_DARK_GRAY);
    w->draw->rect(w, 0, 0, vbe_info->width, 2, 0xf);

    struct time time;
    struct gfx_event event;
    int timedate_length = strlen("00:00:00 00/00/0000");

    w->draw->rect(w, 0, 1, vbe_info->width, TASKBAR_HEIGHT, 30);
    w->draw->rect(w, 0, 0, vbe_info->width, 2, COLOR_VGA_LIGHTER_GRAY+1);
    w->draw->rect(w, 0, TASKBAR_HEIGHT, vbe_info->width, 1, COLOR_VGA_LIGHT_GRAY);
    w->draw->rect(w, 0, TASKBAR_HEIGHT+1, vbe_info->width, 1, 0);

    /* print text for all headers */
    for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
        if(default_taskbar.headers[i].name[0] == 0) continue;
        gfx_button(default_taskbar.headers[i].x + 20, default_taskbar.headers[i].y, default_taskbar.headers[i].w + 16, default_taskbar.headers[i].h, default_taskbar.headers[i].name);
        if(default_taskbar.headers[i].icon != NULL){
            gfx_put_icon16(default_taskbar.headers[i].icon, default_taskbar.headers[i].x+4, default_taskbar.headers[i].y);
        }
    }
    
    while (1){

        gfx_put_icon16(wlan_16, w->inner_width - (timedate_length*8) - 20, 2);

        get_current_time(&time);
        w->draw->rect(w, w->inner_width - (timedate_length*8), 5, timedate_length*8, 10, 30);
        w->draw->textf(w, w->inner_width - (timedate_length*8), 5, COLOR_BLACK,
            "%s%d:%s%d:%s%d %s%d/%s%d/%d",
            TIME_PREFIX(time.hour), time.hour, TIME_PREFIX(time.minute),time.minute,
            TIME_PREFIX(time.second), time.second, TIME_PREFIX(time.day), time.day,
            TIME_PREFIX(time.month), time.month, time.year
        );
        
        /* draw options */
        gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        switch (event.event){
        case GFX_EVENT_MOUSE:{
                /* check if mouse event is inside a header */
                for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
                    if(default_taskbar.headers[i].name[0] == 0) continue;
                    /* clear all extended options*/
                    if(default_taskbar.headers[i].extended){
                        taskbar_hdr_opt_event(w, &default_taskbar.headers[i], event.data, event.data2);
                        default_taskbar.headers[i].extended = false;
                        w->draw->rect(w,
                            default_taskbar.headers[i].x,
                            default_taskbar.headers[i].y+20,
                            TASKBAR_EXT_OPT_WIDTH, 
                            TASKBAR_EXT_OPT_HEIGHT,
                            COLOR_TRANSPARENT
                        );
                    }
                }

                for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
                    /* continue for empty headers */
                    if(default_taskbar.headers[i].name[0] == '\0') continue;

                    dbgprintf("Checking header %x\n", default_taskbar.headers[i].name);

                    /* I have no idea why this needs to be here... :/ */
                    if(default_taskbar.headers[i].extended){
                        taskbar_hdr_opt_event(w, &default_taskbar.headers[i], event.data, event.data2);
                    }

                    taskbar_hdr_event(w, &default_taskbar.headers[i], event.data, event.data2);
                }
            }
            dbgprintf("Mouse event: %d %d\n", event.data, event.data2);
            break;
        default:
            break;
        }

        kernel_yield();
    }
}
EXPORT_KTHREAD(taskbar);


static void __callback taskbar_wolfstein()
{
    int pid = pcb_create_process("/bin/wolf3d.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "wolf.o");
}

static void __callback taskbar_terminal()
{
    dbgprintf("Starting terminal %d\n", __cli_cnt);
    start("shell",  0, NULL);
    dbgprintf("Started terminal %d\n", __cli_cnt);
}

static void __callback taskbar_finder()
{
    int pid = pcb_create_process("/bin/finder.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "finder.o");
}

static void __callback taskbar_editor()
{
    int pid = pcb_create_process("/bin/edit.o", 0, NULL, 0);
	if(pid < 0)
		dbgprintf("%s does not exist\n", "edit.o");
}

static void __callback taskbar_cube()
{
    int pid = pcb_create_process("/bin/cube.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "cube");
}

static void __callback taskbar_colors()
{
    int pid = pcb_create_process("/bin/colors.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "colors");
}

static void __callback taskbar_clock()
{
    start("kclock", 0, NULL);
}

static void __callback taskbar_bg_default()
{
    gfx_raw_background("/imgs/snow.bin");
}

static void __callback taskbar_bg_lotr()
{
    gfx_decode_background_image("/imgs/lotr2.img");
}

static void __callback taskbar_bg_lotr2()
{
    gfx_decode_background_image("/imgs/lotr3.img");
}

static void __callback taskbar_bg_retro()
{
    if(vbe_info->height == 480)
        gfx_raw_background("/imgs/output.bin");
    else
        gfx_decode_background_image("/imgs/retro.img");
}

static void __callback taskbar_users()
{
    int pid = pcb_create_process("/bin/users.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "users.o");
}

static void __callback taskbar_bg_default_color()
{
    gfx_set_background_color(3);
}

static void __callback taskbar_bg_default_color_gray()
{
    gfx_set_background_color(0x17);
}

static void __callback taskbar_bg_calc()
{
    int pid = pcb_create_process("/bin/calc.o", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "calc.o");
}

static void __callback taskbar_sysinf()
{
    start("sysinf", 0, NULL); 
}

static void __callback taskbar_about()
{
    start("about", 0, NULL);
}

static void __callback taskbar_readme()
{
    start("readme", 0, NULL);
}

static void __callback taskbar_workspaces()
{
    start("workspaces", 0, NULL);
}

static void __callback __reboot(int opt)
{
    if(opt == MSGBOX_OK) system_reboot();
}

static void __callback __shutdown(int opt)
{
    if(opt == MSGBOX_OK) system_shutdown();
}

static void __callback taskbar_reboot(){
    struct msgbox* box = msgbox_create(
        MSGBOX_TYPE_INFO,
        MSGBOX_BUTTON_OK | MSGBOX_BUTTON_CANCEL, 
        "Reboot", "Do you want to reboot?",
        &__reboot
    );
    msgbox_show(box);

    //reboot();
}

static void __callback taskbar_shutdown()
{
     struct msgbox* box = msgbox_create(
        MSGBOX_TYPE_INFO,
        MSGBOX_BUTTON_OK | MSGBOX_BUTTON_CANCEL,
        "Shutdown", "Do you want to shutdown?",
        &__shutdown
    );
    msgbox_show(box);
}