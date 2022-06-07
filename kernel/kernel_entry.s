.code32
.section .text.prologue

.global _start
_start:
    movl $stack, %esp
    andl $-16, %esp
    /* Use the number 0xDEADBEEF to check we can use that high addresses. */
    movl $0xDEADBEEF, %eax
    pushl %esp
    pushl %eax
    cli
    call _main

.global _context_switch
_context_switch:
    movl current_running, %eax

    pushfl
    pushal
    # fsave 12(%eax)
    movl %esp, 4(%eax)

    call context_switch

    movl current_running, %eax

    movl 4(%eax), %esp
    # frstor 12(%eax)
    popal
    popfl

    sti
    ret

.global _start_pcb
_start_pcb:
    movl current_running, %eax
    movl 4(%eax), %esp
    sti
    jmp 8(%eax)

.global spin_lock_asm
spin_lock_asm:
again:
    movl %esp, %eax
    addl $4, %eax
    movl (%eax), %eax
    lock bts $0, (%eax)
    jc again
    ret
wait_lock:
    testl $1, (%eax)
    jnz wait_lock
    jmp again
 
.global spin_unlock_asm
spin_unlock_asm:
    movl %esp, %eax
    addl $4, %eax
    movl (%eax), %eax
    lock btr $0, (%eax)
    ret

.section .text
.align 4

.section .bss /* .bss or .data */
.align 32
stack_begin:
    .fill 0x4000
stack:
