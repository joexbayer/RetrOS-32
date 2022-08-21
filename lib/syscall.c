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
#include <syscall_helper.h>
#include <syscall.h>
#include <stdint.h>

int malloc();

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
    int rest = invoke_syscall(SYSCALL_SCRPUT, x, y, c);
}

void print_put(unsigned char c)
{
    int rest = invoke_syscall(SYSCALL_PRTPUT, c, 0, 0);
}