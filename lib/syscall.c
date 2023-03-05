/**
 * @file syscall.c
 * @author Joe Bayer (joexbayer)
 * @brief Systemcall library of userspace programs
 * @version 0.1
 * @date 2022-08-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <syscall_helper.h>
#include <lib/syscall.h>
#include <stdint.h>
#include <util.h>
#include <rtc.h>

int invoke_syscall(int i, int arg1, int arg2, int arg3)
{
    int ret;

    __asm__ volatile ("int $48" /* 48 = 0x30 */
                  : "=a" (ret)
                  : "%0" (i), "b" (arg1), "c" (arg2), "d" (arg3));
    return ret;
}

void screen_put(int x, int y, unsigned char c)
{
    invoke_syscall(SYSCALL_SCRPUT, x, y, c);
}

void print_put(unsigned char c)
{
    invoke_syscall(SYSCALL_PRTPUT, c, 0, 0);
}

void exit()
{
    invoke_syscall(SYSCALL_EXIT, 0, 0, 0);
}

void sleep(int seconds)
{
    invoke_syscall(SYSCALL_SLEEP, seconds, 0, 0);
}

void gfx_create_window(int width, int height)
{
    invoke_syscall(SYSCALL_GFX_WINDOW, width, height, 0);
}

int get_current_time(struct time* time)
{
    return invoke_syscall(SYSCALL_GFX_GET_TIME, (int)time, 0, 0);
}
int gfx_draw_syscall(int option, void* data)
{
    return invoke_syscall(SYSCALL_GFX_DRAW, option, (int)data, 0);
}

int gfx_set_title(char* title)
{
    return invoke_syscall(SYSCALL_GFX_SET_TITLE, (int)title, 0, 0);
}

int open(char* name)
{
    return invoke_syscall(SYSCALL_OPEN, (int)name, 0, 0);
}

int write(int fd, void* buffer, int size)
{
    return invoke_syscall(SYSCALL_WRITE, fd, (int)buffer, size);
}

int read(int fd, void* buffer, int size)
{
    return invoke_syscall(SYSCALL_READ, fd, (int)buffer, size);
}

void* malloc(int size)
{
    return (void*)invoke_syscall(SYSCALL_MALLOC, size, 0, 0);
}

void free(void* ptr)
{
    return invoke_syscall(SYSCALL_MALLOC, (int)ptr, 0, 0);
}

#ifdef __cplusplus
}
#endif
