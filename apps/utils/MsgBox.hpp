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

#define MSGBOX_WIDTH 150
#define MSGBOX_HEIGHT 100

void __msgbox_fn(void* args);

class MsgBox {

public:
    MsgBox(const char* title, const char* message, MsgBoxType type) : mThread(__msgbox_fn, 0) {
        mType = type;
        mTitle = title;
        mMessage = message;
    }

    MsgBoxResult show() {
        mThread.start(this);
        while (isDone() != 0) {
            yield();
        }

        return mResult;
    }

    void close() {
        mRunning = 0;
    }

    volatile char mRunning = 1;
    MsgBoxResult mResult;
    const char* mTitle;
    const char* mMessage;
    MsgBoxType mType;
private:
    Thread mThread;

    volatile char isDone() {
        return mRunning;
    }
};

void __msgbox_fn(void* args){
    MsgBox* box = static_cast<MsgBox*>(args);

    Window win(MSGBOX_WIDTH, MSGBOX_HEIGHT, box->mTitle, 0);
    win.drawContouredRect(0, 0, MSGBOX_WIDTH, MSGBOX_HEIGHT);

    gfx_draw_format_text(10, 30, COLOR_BLACK, "%s", box->mMessage);

    switch (box->mType) {
        case MSGBOX_OK_CANCEL:{
                win.drawContouredRect(10, 10, 50, 20);
                gfx_draw_format_text(20, 16, COLOR_BLACK, "OK");

                win.drawContouredRect(70, 10, 50, 20);
                gfx_draw_format_text(80, 16, COLOR_BLACK, "Cancel");
            }
            break;
        case MSGBOX_OK:
            break;
        case MSGBOX_YES_NO:
            break;
        case MSGBOX_YES_NO_CANCEL:
            break;
        default:
            break;
    }

    struct gfx_event event;
    while (1){
        int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_MOUSE:{
                if (event.data > 10 && event.data < 60 && event.data2 > 10 && event.data2 < 30) {
                    box->mResult = MSGBOX_RESULT_OK;
                    box->close();
                    return;
                } else if (event.data > 70 && event.data < 120 && event.data2 > 10 && event.data2 < 30) {
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