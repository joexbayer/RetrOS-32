#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#include <ksyms.h>

typedef int (*syscall_t) ();
void add_system_call(int index, syscall_t fn);
int system_call(int index, int arg1, int arg2, int arg3);

#define testsd #

#define EXPORT_SYSCALL(index, fn) \
    static void __add_systemcall_##fn() { add_system_call(index, (syscall_t)&fn);} \
    EXPORT_KCTOR(__add_systemcall_##fn)\

#endif /* __SYSCALLS_H */