.code32
.section .text.prologue

.global _start
_start:
    movl $stack, %esp
    /* Use the number 0xDEADBEEF to check we can use that high addresses. */
    movl $0xDEADBEEF, %eax
    pushl %esp
    pushl %eax
    cli
    call _main

.section .bss
.align 16
stack_bottom:
.skip 8128 # 16 KiB
stack: