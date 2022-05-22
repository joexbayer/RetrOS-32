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
		push $\index
		jmp isr_entry
.endm

ISR_NO_ERR 0
ISR_NO_ERR 32
ISR_NO_ERR 33
ISR_NO_ERR 43

isr_entry:
	pusha

	push %eax
    call isr_handler
	pop %eax

    popa
    add $8, %esp
    iret