#include <kthreads.h>
#include <util.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <gfx/composition.h>
#include <gfx/theme.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <kutils.h>
#include <vbe.h>

#include <lib/icons.h>

struct boot_info {
		unsigned int extended_memory_low;
		unsigned int extended_memory_high;
	} *total_memory;


void __kthread_entry login()
{   
    struct window* w = gfx_new_window(275, 100, GFX_NO_OPTIONS);
    if(w == NULL){
        warningf("Failed to create window for login");
        return;
    }

    total_memory = (struct boot_info*) (0x7e00);

    struct unit unit = calculate_size_unit(total_memory->extended_memory_low * 1024);
    struct unit unit2 = calculate_size_unit(total_memory->extended_memory_high * 64 * 1024);

    /* set title */
    kernel_gfx_set_title("Welcome to RetrOS-32");

    /* put it in the middle of screen based on window size */
    w->ops->move(w, (vbe_info->width - w->width) / 2, (vbe_info->height - w->height) / 2);

    w->draw->rect(w, 0, 0, 275, 100, 30);
    w->draw->box(w, 10, 10, 275-20, 100-20, 30+1);

    /* draw text */
    w->draw->text(w, 10+10, 10+10,  "Welcome to RetrOS", 0x0);
    w->draw->text(w, 10+10, 10+10+10,  "Please login", 0x0);

    /* ok button */
    gfx_button(10+10, 10+10+10+10, 50, 20, "OK");

    /* icon to the right middle 32x32 */
    gfx_put_icon32(computer_icon, 275-64-16, 10+20+5);

    /* draw memory info */
    w->draw->textf(w, 10+10, 10+10+10+10+20+10,0x0,"Memory: %d %s", unit.size, unit.unit);
    w->draw->textf(w, 10+10, 10+10+10+10+20+10+10, 0x0, "Extended Memory: %d %s", unit2.size, unit2.unit);



    while (1)
    {
        struct gfx_event event;
			int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
			if(ret == -1) continue;

			switch (event.event){
			case GFX_EVENT_MOUSE:{
                    /* check if OK is clicked x = event.data, y = event.data2 */
                    if(event.data > 10+10 && event.data < 10+10+50 && event.data2 > 10+10+10+10 && event.data2 < 10+10+10+10+20){
                        /* OK is clicked */
                        start("kclock", 0, NULL);
                        pid_t taskbar = start("taskbar", 0, NULL);
                        if(taskbar <= 0){
                            warningf("Failed to start taskbar");
                        }

                        dbgprintf("Taskbar started with pid: %d\n", taskbar);

                        gfx_set_taskbar(taskbar);
                        return;
                    }
                }				
				break;
			default:
				break;
			}

    }
}
EXPORT_KTHREAD(login);