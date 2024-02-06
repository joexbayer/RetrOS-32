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
#include <gfx/component.h>
#include <script.h>

#include <user.h>
#include <usermanager.h>
#include <lib/icons.h>
#include <conf.h>

struct gfx_input_manager input_manager = {0};

static void __callback __login_create_virt_disk(int opt){
    if(opt == MSGBOX_OK){
        virtual_disk_attach();
        dbgprintf("Creating virtual disk\n");
    }
}

static int login_startup(struct user* usr){

    $process->current->user = usr;

    exec_cmd("sh /sysutil/startup.sh");

    if(!disk_attached()){
        struct msgbox* box = msgbox_create(
            MSGBOX_TYPE_WARNING,
            MSGBOX_BUTTON_OK | MSGBOX_BUTTON_CANCEL,
            "No disk attached", " Create virtual disk?",
            __login_create_virt_disk
        );
        msgbox_show(box);
    }

    gfx_set_taskbar(0);
    kernel_exit();
    return;
}

void __kthread_entry login()
{   
    /* check if logon is disabled and a default usr is set. */ 
    char* logon = config_get_value("system", "logon");
    if(logon != NULL){
       if(strcmp(logon, "disabled") == 0){
            char* username = config_get_value("system", "user");
            if(username != NULL){
                struct user* usr = $services->usermanager->ops->get($services->usermanager, username);
                if(usr == NULL){
                    warningf("Failed to get user");
                    return;
                }

                login_startup(usr);
                return;
            }
        }
    }

    struct window* w = gfx_new_window(275, 100, GFX_NO_OPTIONS);
    if(w == NULL){
        warningf("Failed to create window for login");
        return;
    }

    struct unit unit = calculate_size_unit($kernel->boot_info->extended_memory_low * 1024);
    struct unit unit2 = calculate_size_unit($kernel->boot_info->extended_memory_high * 64 * 1024);

    struct usermanager* usermanager= $services->usermanager;

    /* set title */
    kernel_gfx_set_title("Welcome to RetrOS-32");

    /* put it in the middle of screen based on window size */
    w->ops->move(w, (vbe_info->width - w->width) / 2, (vbe_info->height - w->height) / 2);

    w->draw->rect(w, 0, 0, 275, 100, 30);
    w->draw->box(w, 10, 10, 275-20, 100-20, 30+1);

    /* draw text */
    w->draw->text(w, 10+10, 10+10,  "Welcome to RetrOS", 0x0);
    //w->draw->text(w, 10+10, 10+10+10,  "Please login", 0x0);

    gfx_input_manager_add(&input_manager, (struct gfx_input){
        .x = 10+10,
        .y = 10+10+10+10,
        .width = 120,
        .height = 12,
        .placeholder = "Username",
        .clicked = 0,
        .buffer_size = 0
    });

    gfx_input_manager_add(&input_manager, (struct gfx_input){
        .x = 10+10,
        .y = 10+10+10+10+10,
        .width = 120,
        .height = 12,
        .placeholder = "Password",
        .clicked = 0,
        .buffer_size = 0
    });

    /* ok button */
    gfx_button(10+10+120, 10+10+10+10, 50, 20, "OK");

    /* icon to the right middle 32x32 */
    gfx_put_icon32(computer_icon, 275-64-16, 10+20+5);

    /* draw memory info */
    w->draw->textf(w, 10+10, 10+10+10+10+20+10,0x0,"Memory: %d %s", unit.size, unit.unit);
    w->draw->textf(w, 10+10, 10+10+10+10+20+10+10, 0x0, "Extended Memory: %d %s", unit2.size, unit2.unit);

    while (1){
        gfx_input_draw(w, &input_manager);
        gfx_commit();
        
        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;

        gfx_input_event(&input_manager, &event);

        switch (event.event){   
        case GFX_EVENT_MOUSE:{
                /* check if OK is clicked x = event.data, y = event.data2 */
                if(event.data > 10+10+120 && event.data < 10+10+120+50 && event.data2 > 10+10+10+10 && event.data2 < 10+10+10+10+20){
                    /* check if username and password is correct */
                    struct user* usr = usermanager->ops->authenticate(usermanager,
                        input_manager.inputs[0].buffer,
                        input_manager.inputs[1].buffer
                    );
                    if(usr == NULL){
                        warningf("Failed to authenticate user\n");
                        break;
                    }

                    login_startup(usr);
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
