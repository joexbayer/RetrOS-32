/**
 * @file msbox.c
 * @author Joe Bayer (joexbayer)
 * @brief Generic message box
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <msgbox.h>

#include <memory.h>
#include <kthreads.h>
#include <scheduler.h>
#include <gfx/window.h>
#include <gfx/events.h>
#include <gfx/gfxlib.h>
#include <lib/icons.h>
#include <vbe.h>
#include <lib/display.h>

static void __kthread_entry __msgbox(int argc, char* argv[]);

struct msgbox*  msgbox_create(msgbox_type_t type, int flags, char* title, char* message, void (*callback)(int))
{
    struct msgbox* box = create(struct msgbox);
    if(box == NULL) return NULL;

    box->type = type;
    box->title = title;
    box->message = message;
    box->callback = callback;
    box->flags = flags;

    return box;
}

void msgbox_show(struct msgbox* box)
{
    if(box == NULL) return;

    char buffer[256];
    itoa((int)box, buffer);
    pcb_create_kthread(__msgbox, "msgbox", ARGS("msgbox", buffer));
}

#define BUTTON(x, y, _text) \
    w->draw->box(w, x, y, 50, 20, 30); \
    w->draw->text(w, x + (25-(strlen(_text)*8)/2) , y + 4, _text, 0);

static void __kthread_entry __msgbox(int argc, char* argv[])
{
    const int width = 250;
    const int height = 100;

    if(argc < 2) return;

    struct msgbox* box = (struct msgbox*)atoi(argv[1]);

    struct window* w = gfx_new_window(width, height, GFX_NO_OPTIONS);
    if(w == NULL){
        warningf("Failed to create window for msgbox");
        return;
    }

    /* move to center */
    struct display_info display;
    display_get_info(&display);

    w->ops->move(w,  display.width/2 - width/2, display.height/2 - height/2);

    kernel_gfx_set_title(box->title);

    w->draw->rect(w, 0, 0, width, height, 30);

    w->draw->text(w, 44, 32, box->message, 0);

    switch (box->type){
    case MSGBOX_TYPE_INFO:
        gfx_put_icon32(info_32, 10, 20);
        break;
    case MSGBOX_TYPE_WARNING:
        gfx_put_icon32(warning_32, 10, 20);
        break;
    case MSGBOX_TYPE_ERROR:
        gfx_put_icon32(error_32, 10, 20);
        break;
    }

    if(HAS_FLAG(box->flags, MSGBOX_BUTTON_OK)){
        BUTTON(37, 70, "OK");
    }

    if(HAS_FLAG(box->flags, MSGBOX_BUTTON_YESNO)){
        BUTTON(37, 70, "YES");
        BUTTON(162, 70, "NO");
    }

    if(HAS_FLAG(box->flags, MSGBOX_BUTTON_CANCEL)){
        BUTTON(162, 70, "CANCEL");
    }


    while (1){
        struct gfx_event event;
        int ret = gfx_event_loop(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_EXIT:
            kfree(box);
            return;

        case GFX_EVENT_MOUSE:{
            if(HAS_FLAG(box->flags, MSGBOX_BUTTON_OK)){
                if(event.data > 37 && event.data < 37+50 && event.data2 > 70 && event.data2 < 70+20){
                    box->result = MSGBOX_OK;
                    if(box->callback != NULL) box->callback(box->result);
                    kfree(box);
                    return;
                }
            }

            if(HAS_FLAG(box->flags, MSGBOX_BUTTON_YESNO)){
                if(event.data > 37 && event.data < 37+50 && event.data2 > 70 && event.data2 < 70+20){
                    box->result = MSGBOX_YES;
                    if(box->callback != NULL) box->callback(box->result);
                    kfree(box);
                    return;
                }

                if(event.data > 162 && event.data < 162+50 && event.data2 > 70 && event.data2 < 70+20){
                    box->result = MSGBOX_NO;
                    if(box->callback != NULL) box->callback(box->result);
                    kfree(box);
                    return;
                }
            }

            if(HAS_FLAG(box->flags, MSGBOX_BUTTON_CANCEL)){
                if(event.data > 162 && event.data < 162+50 && event.data2 > 70 && event.data2 < 70+20){
                    box->result = MSGBOX_CANCEL;
                    if(box->callback != NULL) box->callback(box->result);
                    kfree(box);
                    return;
                }
            }
        } break;
        }
    }
}
