#include <kthreads.h>
#include <pcb.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <vbe.h>
#include <colors.h>
#include <rtc.h>
#include <timer.h>
#include <gfx/component.h>

#define TASKBAR_MAX_OPTIONS 10
#define TASKBAR_MAX_HEADERS 4
#define TIME_PREFIX(unit) unit < 10 ? "0" : ""

struct taskbar_option {
    struct {
        const char* name;
        struct {
            const char* name;
            const char* path; 
        } options[TASKBAR_MAX_OPTIONS];
        int x, y, w, h;
    } headers[TASKBAR_MAX_HEADERS];
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
                    .name = "Shutdown",
                    .path = "/"
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
                    .name = "Terminal",
                    .path = "/home"
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
                    .name = "Test",
                    .path = "/"
                },
            }
        },
    }
};

void __kthread_entry taskbar()
{
    struct window* w = gfx_new_window(800, 100, GFX_IS_IMMUATABLE | GFX_HIDE_HEADER | GFX_HIDE_BORDER | GFX_IS_TRANSPARENT);
    if(w == NULL){
        warningf("Failed to create window for taskbar");
        return;
    }

    w->ops->move(w, 0, 0);
    w->draw->rect(w, 0, 0, 800, 18, COLOR_VGA_LIGHTER_GRAY);
    w->draw->rect(w, 0, 17, 800, 1, COLOR_VGA_DARK_GRAY);
    w->draw->rect(w, 0, 0, 800, 2, 0xf);

    struct time time;
    struct gfx_event event;
    while (1){

        w->draw->rect(w, 0, 1, 800, 16, COLOR_VGA_LIGHTER_GRAY);

        get_current_time(&time);
        w->draw->textf(w, w->inner_width - 22*8, 5, COLOR_BLACK, "%s%d:%s%d:%s%d %s%d/%s%d/%d", TIME_PREFIX(time.hour), time.hour, TIME_PREFIX(time.minute), time.minute, TIME_PREFIX(time.second), time.second, TIME_PREFIX(time.day), time.day, TIME_PREFIX(time.month), time.month, time.year);

        /* print text for all headers */
        for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
            w->draw->text(w, default_taskbar.headers[i].x + 4, 5, default_taskbar.headers[i].name, COLOR_BLACK);
        }

        gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        switch (event.event){
        case GFX_EVENT_MOUSE:{
                /* check if mouse event is inside a header using int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y); */
                for (int i = 0; i < TASKBAR_MAX_HEADERS; i++){
                    if(gfx_point_in_rectangle(default_taskbar.headers[i].x, default_taskbar.headers[i].y, default_taskbar.headers[i].x + default_taskbar.headers[i].w, default_taskbar.headers[i].y + default_taskbar.headers[i].h, event.data, event.data2)){
                        /* draw header */
                        dbgprintf("Clicked header %s\n", default_taskbar.headers[i].name);
                        w->draw->rect(w, default_taskbar.headers[i].x, default_taskbar.headers[i].y, 50, 50, COLOR_VGA_LIGHT_GRAY);
                        w->draw->text(w, default_taskbar.headers[i].x + 4, 5, default_taskbar.headers[i].name, COLOR_BLACK);

                        /* draw options */
                        for (int j = 0; j < TASKBAR_MAX_OPTIONS; j++){
                            if(default_taskbar.headers[i].options[j].name == NULL) break;
                            w->draw->text(w, default_taskbar.headers[i].x + 4, 5, default_taskbar.headers[i].options[j].name, COLOR_BLACK);
                        }
                    } else {
                        /* draw header */
                        w->draw->rect(w, default_taskbar.headers[i].x, default_taskbar.headers[i].y, default_taskbar.headers[i].w, default_taskbar.headers[i].h, COLOR_VGA_LIGHTER_GRAY);
                        w->draw->text(w, default_taskbar.headers[i].x + 4, 5, default_taskbar.headers[i].name, COLOR_BLACK);
                    }
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