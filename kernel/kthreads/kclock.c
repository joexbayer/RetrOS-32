/**
 * @file kclock.c
 * @author Joe Bayer (joexbayer)
 * @brief Clock for kernel.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kthreads.h>
#include <libc.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <gfx/theme.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <timer.h>
#include <terminal.h>
#include <kutils.h>
#include <scheduler.h>
#include <math.h>

#define center_x(size) ((110/2) - ((size*8)/2))

static char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

void __kthread_entry kclock(int argc, char* argv[])
{   
    if(argc != 0){
        for (int i = 0; i < argc; i++){
            dbgprintf("kclock: argv[%d] = %s\n", i, argv[i]);
        }
        
    }

    int angle_id;
    struct time now;
    struct gfx_theme* theme;

    int timestamp = 0;
    timestamp = 0;
    
    struct window* w = gfx_new_window(110, 140, 0);
    if(w == NULL){
        warningf("Failed to create window for kclock");
        return;
    }

    kernel_gfx_set_title("Clock");
    w->ops->move(w, 450, 50);
    while (1){
        theme = kernel_gfx_current_theme();

        angle_id = (0.5 * (now.hour%12 * 60 + now.minute) / 6);
        
        get_current_time(&now);

        if(get_time() - timestamp < 2){
            kernel_yield();
            continue;
        }
        timestamp = get_time();

        w->draw->rect(w, 0, 0, 110, 140, 30);

        w->draw->circle(w, 55, 55, 50, COLOR_VGA_LIGHTEST_GRAY, 1);
        w->draw->circle(w, 55, 55, 50, 0, 0);

        /* draw small lines between numbers */
        for(int i = 0; i < 60; i++){
            if(i % 5 == 0) continue;
            w->draw->line(w, 55 + (50*sin_60[i])/1.1, 55+ (50*cos_60[i])/1.1, 55 + (50*sin_60[i])/1.2, 55+ (50*cos_60[i])/1.2, theme->window.text);
        }


        /* 12, 3, 6, 9 numbers */
        w->draw->text(w, center_x(2), 18,  "12", theme->window.text);
        w->draw->text(w, center_x(1), 100-4-8, "6", theme->window.text);
        w->draw->text(w, 18, 55, "9", theme->window.text);
        w->draw->text(w, 100-4-8, 55, "3", theme->window.text);


		w->draw->line(w, 55, 55, 55 + (50*sin_60[angle_id])/1.5, 55+ (50*cos_60[angle_id])/1.5, theme->window.text);
		w->draw->line(w, 55, 55, 55 + (50*sin_60[now.minute])/1.1, 55+ (50*cos_60[now.minute])/1.1, theme->window.text);
		w->draw->line(w, 55, 55, 55 + (50*sin_60[now.second])/1.1, 55+ (50*cos_60[now.second])/1.1, COLOR_VGA_RED);

        w->draw->textf(w, center_x(5), 112, theme->window.text, "%s%d:%s%d", now.hour > 9 ? "" : "0", now.hour, now.minute > 9 ? "" : "0", now.minute);
        w->draw->textf(w, center_x(6), 124, theme->window.text, "%d. %s", now.day, months[ABS(now.month-1)]);

        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_NONBLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_EXIT:
            kernel_exit();
            break;
        default:
            break;
        }

    }
}
EXPORT_KTHREAD(kclock);