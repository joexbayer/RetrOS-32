#include <kthreads.h>
#include <pcb.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <vbe.h>
#include <colors.h>
#include <rtc.h>
#include <timer.h>
#include <gfx/component.h>
#include <kutils.h>
#include <kthreads.h>

#define TASKBAR_MAX_OPTIONS 10
#define TASKBAR_MAX_HEADERS 5

#define TASKBAR_EXT_OPT_WIDTH 100
#define TASKBAR_EXT_OPT_HEIGHT 100

#define TIME_PREFIX(unit) unit < 10 ? "0" : ""

/* prototypes to callbacks */
static void __callback taskbar_terminal();
static void __callback taskbar_editor();
static void __callback taskbar_cube();
static void __callback taskbar_colors();
static void __callback taskbar_clock();

/* prototype to taskbar thread */
static void __kthread_entry taskbar(void);

static struct taskbar_option {
    char name[50];
    void (*callback)(void);

};
static struct taskbar_header {
    char name[50];
    int x, y, w, h;
    bool_t extended;
    struct taskbar_option options[TASKBAR_MAX_OPTIONS];
};

static struct taskbar_options {
    struct taskbar_header headers[TASKBAR_MAX_HEADERS];
} default_taskbar = {
    .headers = {
        {
            .x = 4,
            .y = 0,
            .w = 60,
            .h = 18,
            .name = "Home",
            .options = {
                {
                    .name = "> Shutdown",
                    .callback = NULL
                },
            }
        },
        {
            .x = 60,
            .y = 0,
            .w = 60,
            .h = 18,
            .name = "Open",
            .options = {
                {
                    .name = "> Terminal",
                    .callback = &taskbar_terminal
                },
                {
                    .name = "> Editor",
                    .callback = &taskbar_editor
                },
                {
                    .name = "> Cube",
                    .callback = &taskbar_cube
                },
                {
                    .name = "> Colors",
                    .callback = &taskbar_colors
                },
                {
                    .name = "> Clock",
                    .callback = &taskbar_clock
                },
                
            }

        },
        {
            .x = 120,
            .y = 0,
            .w = 60,
            .h = 18,
            .name = "Help",
            .options = {
                {
                    .name = "> Test",
                    .callback = NULL
                },
                {
                    .name = "> Test",
                    .callback = NULL
                },
                {
                    .name = "> Test",
                   .callback = NULL
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

        w->draw->rect(w, header->x, header->y+18, TASKBAR_EXT_OPT_WIDTH, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_LIGHTER_GRAY);
        /* draw border around previous rect in light gray */
        w->draw->rect(w, header->x, header->y+18+TASKBAR_EXT_OPT_HEIGHT-2, TASKBAR_EXT_OPT_WIDTH, 1, COLOR_VGA_DARK_GRAY);
        w->draw->rect(w, header->x, header->y+18, 1, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_DARK_GRAY);
        w->draw->rect(w, header->x+TASKBAR_EXT_OPT_WIDTH-1, header->y+18, 1, TASKBAR_EXT_OPT_HEIGHT, COLOR_VGA_DARK_GRAY);
        
        header->extended = true;

        /* draw options */
        for (int j = 0; j < TASKBAR_MAX_OPTIONS; j++){
            if(header->options[j].name == NULL) break;
            w->draw->text(w, header->x+4, header->y+18 + (j*10) + 4, header->options[j].name, COLOR_BLACK);
            //w->draw->rect(w, header->x, header->y+18 + (j*8) +4+9, TASKBAR_EXT_OPT_WIDTH, 1, COLOR_VGA_DARK_GRAY);
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
        if(header->options[j].name == NULL) break;
        
        if(gfx_point_in_rectangle(
                header->x+4, /* x, 4 padding */
                header->y+18 + (j*10) + 4, /* y, 18 offset from header */
                header->x+4 + TASKBAR_EXT_OPT_WIDTH, /* width */
                header->y+18 + (j*10) + 4 + 8, /* height */
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
    struct window* w = gfx_new_window(800, 200, GFX_IS_IMMUATABLE | GFX_HIDE_HEADER | GFX_HIDE_BORDER | GFX_IS_TRANSPARENT);
    if(w == NULL){
        warningf("Failed to create window for taskbar");
        return;
    }

    w->ops->move(w, 0, 0);
    w->draw->rect(w, 0, 17, 800, 1, COLOR_VGA_DARK_GRAY);
    w->draw->rect(w, 0, 0, 800, 2, 0xf);

    struct time time;
    struct gfx_event event;
    while (1){

        /* draw background */
        w->draw->rect(w, 0, 1, 800, 16, COLOR_VGA_LIGHTER_GRAY);

        /* draw time */
        get_current_time(&time);
        w->draw->textf(w, w->inner_width - 22*8, 5, COLOR_BLACK,
            "%s%d:%s%d:%s%d %s%d/%s%d/%d",
            TIME_PREFIX(time.hour), time.hour, TIME_PREFIX(time.minute),time.minute,
            TIME_PREFIX(time.second), time.second, TIME_PREFIX(time.day), time.day,
            TIME_PREFIX(time.month), time.month, time.year
        );

        /* print text for all headers */
        for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
            w->draw->text(w, default_taskbar.headers[i].x + 4, 5, default_taskbar.headers[i].name, COLOR_BLACK);
        }

        /* draw options */
        gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        switch (event.event){
        case GFX_EVENT_MOUSE:{
                /* check if mouse event is inside a header */
                for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
                    if(default_taskbar.headers[i].name == NULL) continue;
                    /* clear all extended options*/
                    if(default_taskbar.headers[i].extended){
                        taskbar_hdr_opt_event(w, &default_taskbar.headers[i], event.data, event.data2);
                        default_taskbar.headers[i].extended = false;
                        w->draw->rect(w,
                            default_taskbar.headers[i].x,
                            default_taskbar.headers[i].y+18,
                            TASKBAR_EXT_OPT_WIDTH, 
                            TASKBAR_EXT_OPT_HEIGHT,
                            COLOR_TRANSPARENT
                        );
                    }
                }

                for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
                    /* continue for empty headers */
                    if(default_taskbar.headers[i].name == NULL) continue;

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


static void __callback taskbar_terminal()
{
    start("shell");
}

static void __callback taskbar_editor()
{
    int pid = pcb_create_process("/bin/edit.o", 0, NULL, 0);
	if(pid < 0)
		dbgprintf("%s does not exist\n", "edit.o");
}

static void __callback taskbar_cube()
{
    int pid = pcb_create_process("/bin/cube", 0, NULL, 0);
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
    int pid = pcb_create_process("/bin/clock", 0, NULL, 0);
    if(pid < 0)
        dbgprintf("%s does not exist\n", "clock");
}