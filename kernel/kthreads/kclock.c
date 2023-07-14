#include <kthreads.h>
#include <util.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <gfx/theme.h>
#include <gfx/window.h>
#include <timer.h>
#include <terminal.h>
#include <kutils.h>

#define center_x(size) ((110/2) - ((size*8)/2))

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

void __kthread_entry kclock()
{   
    int angle_id;
    struct time now;
    struct gfx_theme* theme;
    
    struct window* w = gfx_new_window(110, 140, 0);
    if(w == NULL){
        warningf("Failed to create window for kclock");
        return;
    }

    w->ops->move(w, 450, 50);
    while (1)
    {
        theme = kernel_gfx_current_theme();
        angle_id = (0.5 * (now.hour%12 * 60 + now.minute) / 6);
        
        get_current_time(&now);

        w->draw->rect(w, 0, 0, 110, 140, 30);

        w->draw->circle(w, 55, 55, 50, COLOR_VGA_LIGHTEST_GRAY, 1);
        w->draw->circle(w, 55, 55, 50, 0, 0);

		w->draw->line(w, 55, 55, 55 + (50*sin_60[angle_id])/1.5, 55+ (50*cos_60[angle_id])/1.5, theme->window.text);
		w->draw->line(w, 55, 55, 55 + (50*sin_60[now.minute])/1.1, 55+ (50*cos_60[now.minute])/1.1, theme->window.text);
		w->draw->line(w, 55, 55, 55 + (50*sin_60[now.second])/1.1, 55+ (50*cos_60[now.second])/1.1, COLOR_VGA_RED);

        w->draw->textf(w, center_x(5), 112, theme->window.text, "%s%d:%s%d", now.hour > 9 ? "" : "0", now.hour, now.minute > 9 ? "" : "0", now.minute);
        w->draw->textf(w, center_x(6), 124, theme->window.text, "%d. %s", now.day, months[now.month]);

        kernel_sleep(100);

        twritef("Time: %d:%d:%d\n", now.hour, now.minute, now.second);
    }
}
EXPORT_KTHREAD(kclock);