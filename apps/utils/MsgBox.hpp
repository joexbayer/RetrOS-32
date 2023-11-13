#ifndef MSGBOX_HPP
#define MSGBOX_HPP

#include <utils/Graphics.hpp>
#include <lib/syscall.h>
#include <utils/Thread.hpp>
#include <lib/printf.h>

typedef enum _msgbox_types {
    MSGBOX_OK,
    MSGBOX_OK_CANCEL,
    MSGBOX_YES_NO,
    MSGBOX_YES_NO_CANCEL
} MsgBoxType;

typedef enum _msgbox_results {
    MSGBOX_RESULT_OK,
    MSGBOX_CANCEL,
    MSGBOX_YES,
    MSGBOX_NO
} MsgBoxResult;

#define MSGBOX_WIDTH 170
#define MSGBOX_HEIGHT 100

/* Forward declaration of threading function */
void __msgbox_thread_fn(void* args);

class MsgBox {
public:
    MsgBox(const char* title, const char* message, MsgBoxType type) : mThread(__msgbox_thread_fn, 0) {
        mType = type;
        mTitle = title;
        mMessage = message;
    }

    MsgBoxResult show() {
        /* Start the thread */
        mThread.start(this);

        /* Wait for the thread to finish */
        while (isRunning()) {
            yield();
        }

        return mResult;
    }

    void close() {
        mRunning = 0;
    }

    /* public variables used by thread */
    const char* mTitle;
    const char* mMessage;

    MsgBoxResult mResult;
    MsgBoxType mType;
private:
    Thread mThread;

    volatile char mRunning = 1;

    volatile char isRunning() {
        return mRunning == 1;
    }
};

/**
 * @brief Thread function for the message box
 * Handles the drawing of the message box and the user input
 * @param args Pointer to the MsgBox object
 */
void __msgbox_thread_fn(void* args)
{
    MsgBox* box = static_cast<MsgBox*>(args);

    Window win(MSGBOX_WIDTH, MSGBOX_HEIGHT, box->mTitle, 0);
    win.drawContouredRect(0, 0, MSGBOX_WIDTH, MSGBOX_HEIGHT);

    gfx_draw_format_text(10, 30, COLOR_BLACK, "%s", box->mMessage);

    switch (box->mType) {
        case MSGBOX_OK_CANCEL:{
                win.drawContouredRect(10, 70, 50, 20);
                gfx_draw_format_text(20, 76, COLOR_BLACK, "OK");

                win.drawContouredRect(110, 70, 50, 20);
                gfx_draw_format_text(120, 76, COLOR_BLACK, "Cancel");
            }
            break;
        case MSGBOX_OK:{
                win.drawContouredRect(60, 70, 50, 20);
                gfx_draw_format_text(70, 76, COLOR_BLACK, "OK");
            }
            break;
        case MSGBOX_YES_NO:{
                win.drawContouredRect(10, 70, 50, 20);
                gfx_draw_format_text(20, 76, COLOR_BLACK, "Yes");

                win.drawContouredRect(110, 70, 50, 20);
                gfx_draw_format_text(120, 76, COLOR_BLACK, "No");
            }
            break;
        case MSGBOX_YES_NO_CANCEL:{
                win.drawContouredRect(10, 70, 50, 20);
                gfx_draw_format_text(20, 76, COLOR_BLACK, "Yes");

                win.drawContouredRect(60, 70, 50, 20);
                gfx_draw_format_text(70, 76, COLOR_BLACK, "No");

                win.drawContouredRect(110, 70, 50, 20);
                gfx_draw_format_text(120, 76, COLOR_BLACK, "Cancel");
            }
            break;
        default:
            break;
    }

    struct gfx_event event;
    while (1){
        int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;
        if (event.event != GFX_EVENT_MOUSE) continue;

        switch (box->mType){
        case MSGBOX_OK_CANCEL:{
                if (event.data > 10 && event.data < 60 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_RESULT_OK;
                    box->close();
                    return;
                } else if (event.data > 110 && event.data < 160 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_CANCEL;
                    box->close();
                    return;
                }
            }
            break;
        case MSGBOX_OK:{
                if (event.data > 60 && event.data < 110 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_RESULT_OK;
                    box->close();
                    return;
                }
            }
            break;
        case MSGBOX_YES_NO:{
                if (event.data > 10 && event.data < 60 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_YES;
                    box->close();
                    return;
                } else if (event.data > 110 && event.data < 160 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_NO;
                    box->close();
                    return;
                }
            }
            break;
        case MSGBOX_YES_NO_CANCEL:{
                if (event.data > 10 && event.data < 60 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_YES;
                    box->close();
                    return;
                } else if (event.data > 60 && event.data < 110 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_NO;
                    box->close();
                    return;
                } else if (event.data > 110 && event.data < 160 && event.data2 > 70 && event.data2 < 90) {
                    box->mResult = MSGBOX_CANCEL;
                    box->close();
                    return;
                }
            }
            break;
        default:
            break;
        }
    }
}

#endif // !MSGBOX_HPP