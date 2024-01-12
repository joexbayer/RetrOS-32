/**
 * @file login.c
 * @author Joe Bayer (joexbayer)
 * @brief Login screen kthread
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kutils.h>
#include <kthreads.h>
#include <kernel.h>
#include <libc.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <gfx/composition.h>
#include <gfx/theme.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <kutils.h>
#include <vbe.h>
#include <msgbox.h>
#include <virtualdisk.h>
#include <diskdev.h>

#include <lib/icons.h>

static void __callback __login_create_virt_disk(int opt){
    if(opt == MSGBOX_OK){
        virtual_disk_attach();
        dbgprintf("Creating virtual disk\n");
    }
}

void __kthread_entry login()
{   
    struct window* w = gfx_new_window(275, 100, GFX_NO_OPTIONS);
    if(w == NULL){
        warningf("Failed to create window for login");
        return;
    }

    struct unit unit = calculate_size_unit($kernel->boot_info->extended_memory_low * 1024);
    struct unit unit2 = calculate_size_unit($kernel->boot_info->extended_memory_high * 64 * 1024);

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

    gfx_commit();
    while (1){
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

                    if(!disk_attached()){
                        struct msgbox* box = msgbox_create(
                            MSGBOX_TYPE_WARNING,
                            MSGBOX_BUTTON_OK | MSGBOX_BUTTON_CANCEL,
                            "No disk attached", " Create virtual disk?",
                            __login_create_virt_disk
                        );
                        msgbox_show(box);
                    }

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
