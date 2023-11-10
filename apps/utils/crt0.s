.code32
.global _start
_start:
    /* if edx is not 0, then  we are a thread and wish to jump to edz */
    cmpl $0, %edx
    je call_main
    /* Otherwise, call the function pointed to by edx */
    pushl %ecx
    pushl %ebx
    call *%edx
    jmp end_thread

call_main:
    pushl %ecx
    pushl %ebx
    call main

end_thread:
    /* Exit syscall or cleanup */
    call exit