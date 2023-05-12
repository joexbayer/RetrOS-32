/**
 * @file ipc.c
 * @author Joe Bayer (joexbayer)
 * @brief API for inter process communication (IPC)
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <ipc.h>
#include <memory.h>
#include <scheduler.h>
#include <bitmap.h>

/* Potentionally move to dynamic allocation? */
#define MAX_MESSAGE_BOXES 10
#define IPC_MAX_SIGNALS 255

static struct message_box msg_boxes[MAX_MESSAGE_BOXES];
static bitmap_t msg_bitmap;

static signal_value_t signal_values[IPC_MAX_SIGNALS];

void signal_wait(int sig)
{
    WAIT(signal_values[sig] == 0);
    signal_values[sig] = 0;
}

void signal(int sig)
{
    signal_values[sig] = 1;
}

/**
 * @brief Creates new message in message box with given ID
 * 
 * @param id 
 * @param data 
 * @param size 
 * @return int (less than 0 on error.)
 */
int ipc_msg_push(int id, void* data, int size)
{
    return -1;
}

/**
 * @brief Checks if there is a message in the given msg_box
 * Copies message into given message pointer.
 * @param id of message box
 * @param struct message* message to copy into.
 * @return int (less than 0 on error.)
 */
int ipc_msg_get(int id, struct message* msg)
{
    return -1;
}

int ipc_msg_box_create()
{
    return -1;
}

void ipc_msg_box_init()
{
    msg_bitmap = create_bitmap(MAX_MESSAGE_BOXES);
}

