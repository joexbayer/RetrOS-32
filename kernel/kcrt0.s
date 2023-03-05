.code32
.section .text.prologue

.global _start
_start:
    popl %eax

    movl $stack, %esp
    /* Use the number 0xDEADBEEF to check we can use that high addresses. */
    pushl %esp
    pushl %eax
    cli
    call kernel

.section .bss
.align 16
stack_bottom:
.skip 8128 # 16 KiB
stack: