.code32
.section .text.prologue

.global _start
_start:
    popl %eax

    finit

    movl $stack, %esp

    pushl %esp
    pushl %eax
    cli
    call kernel

.section .data
.align 16
stack_bottom:
.skip 4096 # 16 KiB
stack: