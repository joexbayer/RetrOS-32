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
	pusha

	push %eax
    call isr_handler
	pop %eax

    popa
    add $8, %esp
	
	iret
