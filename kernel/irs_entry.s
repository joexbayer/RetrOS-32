/*
	ISR entry point, followed tutorial:
	http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
*/

#include <arch/gdt.h>

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
ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR 8
ISR_NO_ERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_ERR 17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_ERR 21
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28
ISR_NO_ERR 29
ISR_NO_ERR 30
ISR_NO_ERR 31
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
  /*pushal
  
  pushl %ds
  
  mov $16, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  
  call isr_handler

  pop %eax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

  popal

  popl	%ds

  add $8, %esp*/

  pushal

  pushl	%ds
  pushl	$16
  call	load_data_segments
  addl	$4, %esp

  call isr_handler

  popl	%ds

  popal
  add $8, %esp

  iret

syscall_return_value:
  .long	0
.global _syscall_entry
_syscall_entry:
    cli
    pushfl
    pushal

    pushl	%ds

	  pushl	%edx	/* Arg 3 */
    pushl	%ecx	/* Arg 2 */
    pushl	%ebx	/* Arg 1 */
    pushl	%eax	/* Syscall number */

    pushl	$16
    call	load_data_segments
    addl	$4, %esp

    call system_call
    movl	%eax, (syscall_return_value)
	
    popl	%edx	/* Arg 3 */
    popl	%ecx	/* Arg 2 */
    popl	%ebx	/* Arg 1 */
    popl	%eax	/* Syscall number */
    
    popl	%ds

    popal 
    popfl
    
    movl	(syscall_return_value), %eax

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

    pushal

    pushl	%ds
    pushl	$16
    call	load_data_segments
    addl	$4, %esp

    /* Push error code, and then contents of cr2 */
    movl	(page_fault_error), %eax
    pushl	%eax
    movl	%cr2, %eax
    pushl	%eax

    call page_fault_interrupt

    addl	$8, %esp
    
    popl	%ds
    popal    

    iret
