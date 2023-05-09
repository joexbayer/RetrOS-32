#include <kthreads.h>
#include <util.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <timer.h>

#define center_x(size) ((110/2) - ((size*8)/2))

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

void kclock()
{   
    struct time current_time;
    gfx_new_window(110, 140);

    kernel_gfx_set_position(450, 50);

    while (1)
    {
        get_current_time(&current_time);

        kernel_gfx_draw_rectangle(0, 0, 110, 140, COLOR_VGA_BG);

        int angle_id = (0.5 * (current_time.hour%12 * 60 + current_time.minute) / 6);

		kernel_gfx_draw_line(55, 55, 55 + (50*sin_60[angle_id])/1.5, 55+ (50*cos_60[angle_id])/1.5, COLOR_VGA_FG);
		kernel_gfx_draw_line(55, 55, 55 + (50*sin_60[current_time.minute])/1.1, 55+ (50*cos_60[current_time.minute])/1.1, COLOR_VGA_FG);
		kernel_gfx_draw_line(55, 55, 55 + (50*sin_60[current_time.second])/1.1, 55+ (50*cos_60[current_time.second])/1.1, COLOR_VGA_RED);

        kernel_gfx_draw_circle(55, 55, 50, COLOR_VGA_FG);

        kernel_gfx_draw_format_text(center_x(5), 112, COLOR_VGA_FG, "%s%d:%s%d", current_time.hour > 9 ? "" : "0", current_time.hour, current_time.minute > 9 ? "" : "0", current_time.minute);
        kernel_gfx_draw_format_text(center_x(6), 124, COLOR_VGA_FG, "%d. %s", current_time.day, months[current_time.month]);

        kernel_sleep(100);
    }
}
EXPORT_KTHREAD(kclock);