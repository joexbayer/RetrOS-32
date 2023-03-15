.code32
.section .text.prologue

.global tlb_flush_addr
.text
/* Flush the TLB of the addr given as a argument */
tlb_flush_addr:
  movl 4(%esp), %eax
  invlpg (%eax)
  ret

.text
.globl load_page_directory
/* Load the page directory passed as argument */
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
/* Enabales paging */
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
/* Entry and Exit for context_switch() function */
_context_switch:
    cli
    pushfl
    pushal
    
    movl current_running, %eax

    fnsave 24(%eax)
    movl %esp, 4(%eax)
    movl %ebp, 0(%eax)

    /* check if process or kthread */
    cmpl $0, 20(%eax)
    je	skip
    
    /* use kernel stack for processes */
    movl 12(%eax), %esp
    movl 16(%eax), %ebp

skip:
    call context_switch
    movl current_running, %eax

    /* Restore context */
    movl 0(%eax), %ebp
    movl 4(%eax), %esp
    frstor 24(%eax)
    popal
    popfl

    sti
    ret

.global _start_pcb
/* Kick starts a PCB by pushing the entry address and returning */
_start_pcb:
    movl current_running, %eax
    movl 4(%eax), %esp
    movl 0(%eax), %ebp
    movl 152(%eax), %ebx
    movl 156(%eax), %ecx
    sti
    pushl 8(%eax)
    # jmp *8(%eax)
    ret

.section .text
.align 4
