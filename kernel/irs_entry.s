/*
	ISR entry point, followed tutorial:
	http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
*/

.code32
.global idt_flush
idt_flush:
	movl 4(%esp), %eax
	lidt (%eax)
	ret

.macro ISR_NO_ERR index
	.global isr\index
	isr\index:
		cli
		push $0
		push $\index
		jmp isr_entry
.endm

.macro ISR_ERR index
	.global isr\index
	isr\index:
		cli
		push $0
		push $\index
		jmp isr_entry
.endm

ISR_NO_ERR 0
ISR_ERR 8
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NO_ERR 15
ISR_NO_ERR 32
ISR_NO_ERR 33
ISR_NO_ERR 34
ISR_NO_ERR 35
ISR_NO_ERR 36
ISR_NO_ERR 37
ISR_NO_ERR 38
ISR_NO_ERR 39
ISR_NO_ERR 40
ISR_NO_ERR 41
ISR_NO_ERR 42
ISR_NO_ERR 43
ISR_NO_ERR 44
ISR_NO_ERR 45
ISR_NO_ERR 46
ISR_NO_ERR 47

isr_entry:
    pushal

	push %eax
    call isr_handler
	pop %eax

    popal
    add $8, %esp
	
	iret

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

page_fault_save:
  .long	0
page_fault_error:
  .long	0
.global _page_fault_entry
_page_fault_entry:
    cli

    movl	%eax, (page_fault_save)
    popl	%eax
    /* Get error code */
    movl	%eax, (page_fault_error)
    movl	(page_fault_save), %eax

    pusha

    /* Push error code, and then contents of cr2 */
    movl	(page_fault_error), %eax
    pushl	%eax
    movl	%cr2, %eax
    pushl	%eax

    call page_fault_interrupt

    addl	$8, %esp
    
    popa    

    iret
