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
extern "C"
{
#endif

#include <syscall_helper.h>
#include <lib/syscall.h>
#include <stdint.h>
#include <libc.h>
#include <rtc.h>

int invoke_syscall(int i, int arg1, int arg2, int arg3)
{
    int ret;

    __asm__ volatile ("int $48" /* 48 = 0x30 */
                  : "=a" (ret)
                  : "%0" (i), "b" (arg1), "c" (arg2), "d" (arg3));
    return ret;
}

void yield()
{
    invoke_syscall(SYSCALL_YIELD, 0, 0, 0);
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

void gfx_create_window(int width, int height, int flags)
{
    void* ptr = (void*) invoke_syscall(SYSCALL_GFX_WINDOW, width, height, flags);
    if (ptr == NULL) {
        return;
    }
}

int get_current_time(struct time* time)
{
    return invoke_syscall(SYSCALL_GFX_GET_TIME, (int)time, 0, 0);
}
int gfx_draw_syscall(int option, void* data, int flags)
{
    return invoke_syscall(SYSCALL_GFX_DRAW, option, (int)data, flags);
}

int gfx_set_title(const char* title)
{
    return invoke_syscall(SYSCALL_GFX_SET_TITLE, (int)title, 0, 0);
}


int gfx_set_header(const char* header)
{
    return invoke_syscall(SYSCALL_GFX_SET_HEADER, (int)header, 0, 0);
}

int open(const char* name, int flags)
{
    return invoke_syscall(SYSCALL_OPEN, (int)name, flags, 0);
}

int write(int fd, void* buffer, int size)
{
    return invoke_syscall(SYSCALL_WRITE, fd, (int)buffer, size);
}

int read(int fd, void* buffer, int size)
{
    return invoke_syscall(SYSCALL_READ, fd, (int)buffer, size);
}

int thread_create(void* entry, void* arg, int flags)
{
    return invoke_syscall(SYSCALL_CREATE_THREAD, (int)entry, (int)arg, flags);
}

void* malloc(int size)
{
    return (void*)invoke_syscall(SYSCALL_MALLOC, size, 0, 0);
}

void free(void* ptr)
{
    invoke_syscall(SYSCALL_FREE, (int)ptr, 0, 0);
}

int fclose(int fd)
{
    return invoke_syscall(SYSCALL_CLOSE, fd, 0, 0);
}

int system(const char* command)
{
    return invoke_syscall(SYSCALL_SYSTEM, (int)command, 0, 0);
}

#ifdef __cplusplus
}
#endif
