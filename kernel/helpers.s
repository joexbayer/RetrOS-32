.code32
.section .text.prologue

.global tlb_flush_addr
.text
tlb_flush_addr:
  movl 4(%esp), %eax
  invlpg (%eax)
  ret

page_fault_14_scratch:
  .long	0
page_fault_14_err:
  .long	0
.global _page_fault_entry
_page_fault_entry:
    cli

    movl	%eax, (page_fault_14_scratch)
    popl	%eax
    /* Get error code */
    movl	%eax, (page_fault_14_err)
    movl	(page_fault_14_scratch), %eax

    pusha

    /* Push error code, and then contents of cr2 */
    movl	(page_fault_14_err), %eax
    pushl	%eax
    movl	%cr2, %eax
    pushl	%eax

    call page_fault_interrupt

    addl	$8, %esp
    
    popa    

    iret

.text
.globl load_page_directory
load_page_directory:
    push %ebp
    mov %esp, %ebp
    mov 8(%esp), %eax
    mov %eax, %cr3
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

syscall_return_value:
  .long	0
.global _syscall_entry
_syscall_entry:
    cli
    push $0
	push $48
    pusha

	pushl	%edx	/* Arg 3 */
    pushl	%ecx	/* Arg 2 */
    pushl	%ebx	/* Arg 1 */
    pushl	%eax	/* Syscall number */

    call system_call
    movl	%eax, (syscall_return_value)
	
    popl	%edx	/* Arg 3 */
    popl	%ecx	/* Arg 2 */
    popl	%ebx	/* Arg 1 */
    popl	%eax	/* Syscall number */
    
    popa    
    
    movl	(syscall_return_value), %eax

    add $8, %esp
    iret

.global _context_switch
_context_switch:

    pushfl
    pushal
    
    movl current_running, %eax

    fnsave 24(%eax)

    movl %esp, 4(%eax)
    movl %ebp, 0(%eax)
    cmpl $0, 20(%eax)
    je	skip
    
    movl 12(%eax), %esp
    movl 16(%eax), %ebp

skip:
    call context_switch
    movl current_running, %eax

    movl 0(%eax), %ebp
    movl 4(%eax), %esp
    frstor 24(%eax)
    popal
    popfl

    sti
    ret

.global _start_pcb
_start_pcb:
    movl current_running, %eax
    movl 4(%eax), %esp
    movl 0(%eax), %ebp  
    sti
    jmp *8(%eax)

.global spin_lock_asm
spin_lock_asm:
again:
    movl %esp, %eax
    addl $4, %eax
    movl (%eax), %eax
    lock btsl $0, (%eax)
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
    lock btrl $0, (%eax)
    ret

.section .text
.align 4