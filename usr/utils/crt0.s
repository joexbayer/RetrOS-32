.code32
.global _start
_start:
    call main

    /* Must be linked with syscalls for this */
    call exit
