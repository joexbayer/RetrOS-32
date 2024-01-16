#ifndef __MSGBOX_H__
#define __MSGBOX_H__

typedef enum msgbox_type {
    MSGBOX_TYPE_INFO,
    MSGBOX_TYPE_WARNING,
    MSGBOX_TYPE_ERROR,
} msgbox_type_t;

typedef enum msgbox_button {
    MSGBOX_BUTTON_OK = 1 << 0,
    MSGBOX_BUTTON_YESNO = 1 << 1,
    MSGBOX_BUTTON_CANCEL = 1 << 2,
} msgbox_button_t;

typedef enum msgbox_result {
    MSGBOX_OK,
    MSGBOX_CANCEL,
    MSGBOX_YES,
    MSGBOX_NO,
    MSGBOX_RETRY,
    MSGBOX_ABORT,
    MSGBOX_IGNORE,
    MSGBOX_CONTINUE,
} msgbox_result_t;

struct msgbox{
    void (*callback)(int);
    struct window* w;

    msgbox_type_t type;
    msgbox_result_t result;

    char* title;
    char* message;
    int flags;
};

struct msgbox*  msgbox_create(msgbox_type_t type, int flags, char* title, char* message, void (*callback)(int));
void            msgbox_destroy(struct msgbox* msgbox);
void            msgbox_show(struct msgbox* msgbox);

#endif // !__M__