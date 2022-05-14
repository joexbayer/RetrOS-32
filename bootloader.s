/*  Bootloader...
 *
 *   Inspiried by stage0.s in tetris-os by JDH
 *   His video: https://www.youtube.com/watch?v=FaILnmUYS_U&t=647s
 *   Github: https://github.com/jdah/tetris-os
 */

.code16
.org 0
.text

.global _start
_start:
    /* Setup a temporary stack */
    movw $0x3000, %sp

    /* Greet user with message. */
    movw $welcome_str, %si
    call print

    /*
        Using int 13h with 42 Extended Reac Sectors from Drive, to read inn sectors.
        The loop will run 20 times, each time reading 0x0040 (64) * 512 bytes = 0x8000 (32768) bytes
        Making sure to increase to the next segment if needed.
     */
    movw $20, %cx /* Set cx 20 as loop variable meaning it will loop 20 times. */

    movw $disk_address_packet, %si
    movw $0x1000, segment
    movw $1, sector

read_loop:
    movb $0x42, %ah /* 0x42 Extended Read Sectors From Drive */
    int $0x13
    # jc error /* Error???*/

    /* Check if still in same segment. */
    addw $64, sector /* Adding the 64 sectors read*/
    addw $0x8000, offset /* Each iteration reads in 0x8000 bytes */
    jnc reading_same_segment

    /* increment segment, reset offset if on different segment */
    addw $0x1000, segment
    movw $0x0000, offset

reading_same_segment:
    /* decrements %cx and loops if nonzero */
    loop read_loop

    /* enable A20 line */
    call set_a20

    /* enable the PE flag */
    movl %cr0, %eax
    orl $0x1, %eax
    movl %eax, %cr0

    /* Setup GDT for 32 land */
    jmp setup_gdt
setup_gdt:
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
    movl $0x3000, %esp

    sti
    
    ljmp $0x8, $enter32

/* Set a20 line from: http://www.brokenthorn.com/Resources/OSDev9.html*/
set_a20: 
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     set_a20

    movb    $0xd1,%al               # 0xd1 -> port 0x64
    outb    %al,$0x64                                                                                

set_a20.2:
    inb     $0x64,%al               # Wait for not busy
    testb   $0x2,%al
    jnz     set_a20.2

    movb    $0xdf,%al               # 0xdf -> port 0x60
    outb    %al,$0x60
    retw

.code32
enter32:
    /* jump to kernel loaded at 0x10000 */
    movl $0x10000, %eax
    jmpl *%eax

inf:
    jmp inf

.code16
error:
    movw $error_str, %si
    call print
    call inf 

/* 	AL = Character, BH = Page Number, BL = Color (only in graphic mode)*/
print:
    xor %bh, %bh
    /* INT 13 needs 0x0E for teletype output*/
    movb $0x0E, %ah
    lodsb

    cmpb $0, %al
    je return

    int $0x10
    jmp print
return:
    ret

/* Strings */
welcome_str:
    .asciz "Starting...\n"
error_str:
    .asciz "Error while reading..\n"


/* Reading in kernel, using  DAP (Disk Address Packet) */
disk_address_packet:
    .byte 0x10 /* size of DAP (set this to 10h) */
    .byte 0x00
    .word 0x0040 /* number of sectors to be read */
offset:
    .word 0x0000 /* segment:offset pointer to the memory buffer to which sectors will be transferred */ 
segment:
    .word 0x0000 /* will be set to $0x1000 */
sector:
    .quad 0x00000000

/* GDT, is needed for 32 bit */
.align 16
gdtp:
    .word gdt_end - gdt_start - 1
    .long gdt_start

.align 16
gdt_start:
gdt_null:
    .quad 0

/*
    Setup descriptor for the flat memory layout
    Giving us access to all the memory.
    https://en.wikipedia.org/wiki/Global_Descriptor_Table
*/
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

/* BOOT SIGNATURE */
.fill 510-(.-_start), 1, 0
.word 0xAA55 /* Do you believe in magic? */
