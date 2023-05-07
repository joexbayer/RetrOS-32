.set ALIGN,    1<<0                         # align loaded modules on page boundaries
.set MEMINFO,  1<<1                         # provide memory map
.set VIDINFO, 1<<2		            # want graphics
.set FLAGS,    ALIGN | MEMINFO | VIDINFO # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0 # header_addr
.long 0 # load_addr
.long 0 # load_end_addr
.long 0 # bss_end_addr
.long 0 # entry_addr
.long 0 # mode_type
.long 1280 # width
.long 1024 # height
.long 8 # depth

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

    pushl %ebx
	call kernel
 
	
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