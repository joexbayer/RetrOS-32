#include <arch/gdt.h>

INIT_EFLAGS = ((0 << 12) | (1 << 9))

.code32
.section .text.prologue

.global tlb_flush_addr
.text
tlb_flush_addr:
  movl 4(%esp), %eax
  invlpg (%eax)
  ret

.text
.globl load_page_directory
load_page_directory:
    push %ebp
    mov %esp, %ebp
    pushl %eax

    mov 12(%esp), %eax
    mov %eax, %cr3

    popl %eax
    mov %ebp, %esp
    pop %ebp
    ret

.text
.globl enable_paging
enable_paging:
    push %ebp
    mov %esp, %ebp

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    
    mov %ebp, %esp
    pop %ebp
    ret

.global _context_switch
_context_switch:
    cli
    addl $1, cli_cnt
    pushfl
    pushal
    
    movl current_running, %eax

    fnsave 24(%eax)

    movl %esp, 12(%eax)
    movl %ebp, 16(%eax)

skip:
    call context_switch
    movl current_running, %eax

    movl 16(%eax), %ebp
    movl 12(%eax), %esp
    frstor 24(%eax)
    popal
    popfl

    sti
    subl $1, cli_cnt
    ret

.global _start_pcb
_start_pcb:
    movl current_running, %eax
    movl 12(%eax), %esp
    
    cmpl $0, 20(%eax)
    je _start_pcb_skip
    pushl	160(%eax)
    pushl	4(%eax)
_start_pcb_skip:
    pushl	$INIT_EFLAGS
    pushl	164(%eax)
    pushl	8(%eax)
    subl $1, cli_cnt
    movw	160(%eax),%ds
    movw	160(%eax),%es
    iret

.section .text
.align 4
