.code32
.global _start
_start:
    pushl %ecx
    pushl %ebx
    call main

    /* Must be linked with syscalls for this */
    call exit
