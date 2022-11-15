#ifndef __SYSCALLS_H
#define __SYSCALLS_H

typedef int (*syscall_t) ();
void add_system_call(int index, syscall_t fn);
int system_call(int index, int arg1, int arg2, int arg3);

#endif /* __SYSCALLS_H */