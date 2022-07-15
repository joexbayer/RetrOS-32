/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */
 

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
 

.section .bss
.align 16

.section .text
.global _start
.type _start, @function
_start:
	
    cli

    lgdt gdtp

    /* Setup GDT descriptor for data, setting registers. */
    movw $(data_descriptor - gdt_start), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
	

    movl $stack, %esp
    andl $-16, %esp
    /* Use the number 0xDEADBEEF to check we can use that high addresses. */
    movl $0xDEADBEEF, %eax
    pushl %esp
    pushl %eax

	call _main
 
	
	cli
1:	hlt
	jmp 1b
 
 /* GDT, is needed for 32 bit */
.align 16
gdtp:
    .word gdt_end - gdt_start - 1
    .long gdt_start

/*
    Setup descriptor for the flat memory layout
    Giving us access to all the memory.
    https://en.wikipedia.org/wiki/Global_Descriptor_Table
*/
.align 16
gdt_start:
gdt_null:
    .quad 0
code_descriptor:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x00
data_descriptor:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10010010
    .byte 0b11001111
    .byte 0x00
gdt_end:

.size _start, . - _start

.section .bss /* .bss or .data */
.align 32
stack_begin:
    .fill 0x4000
stack: