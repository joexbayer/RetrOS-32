.code16
.org 0
.text

.global _start
_start:
    jmp main
    .space 62 /* space for fat16 info */

main:
    cli
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    /* Setup a temporary stack */
    movw $0x3000, %sp
    /* Save Drive number of booted drive. */
    mov %dl, drive_num

    /* Clear screen and set colors */
    mov $0x0600, %ax  /* Scroll up function */
    xor %cx, %cx      /* Top left corner of the window (Column 0, Row 0) */
    mov $0x184F, %dx  /* Bottom right corner of the window (Column 79, Row 24) */
    mov $0x1F, %bh    /* Attribute byte (blue background, white text) */
    int $0x10

    /* Set cursor position to top left */
    xor %dx, %dx      /* Row 0, Column 0 */
    mov $0x02, %ah    /* Function to set cursor position */
    int $0x10


    /* Greet user with message. */
    movw $welcome_str, %si
    call print
    # mov $0x0, %ah
    # int $0x16

    /* Read reserved_blocks */
    movw _start+14, %bx

    movw %bx, %ax      /* Copy %bx to %ax */
    xorw %dx, %dx      /* Clear %dx, now DX:AX contains the value from %bx */
    movw $64, %cx       /* Load divisor value (64 * 512) into %cx */
    divw %cx           /* Divide AX by %cx. Quotient goes in %ax, Remainder in %dx */
    movw %ax, %cx      /* Move the result into %cx */

    /**
     * Using int 13h with 42 Extended Reac Sectors from Drive, to read inn sectors.
     * The loop will run 15 times, each time reading 0x0040 (64) * 512 bytes = 0x8000 (32768) bytes
     * Loading in a total of around 500Kb staying inside of the ram we have.
     * Making sure to increase to the next segment if needed.
     */
    # movw $8, %cx /* Set cx 15 as loop variable meaning it will loop 15 times. */
    movb drive_num, %dl
    movw $disk_address_packet, %si
    movw $0x1000, segment /* Load the kernel at 0x100000 -> segment 0x10000 */
    movw $1, sector

read_loop:
    movb $0x42, %ah /* 0x42 Extended Read Sectors From Drive */
    int $0x13
    jc kill

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

continue:
    call set_video_mode

    /* enable A20 line */
    call set_a20
    /* enable the PE flag */
    movl %cr0, %eax
    orl $0x1, %eax
    movl %eax, %cr0

    /* Setup GDT for 32 land */
    jmp setup_gdt

setup_gdt:
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

    ret                                                                                

set_video_mode:
    /* Print message asking for resolution choice */
    movw $choice_str, %si
    call print

    /* Wait for key press */
    mov $0x00, %ah     /* Function 0x00 - Get Keypress */
    int $0x16          /* BIOS Keyboard Services */

    /* Check if pressed key is '1' or '2' */
    cmp $'1', %al
    je set_640x480
    cmp $'2', %al
    je set_800x600

    jmp set_video_mode

set_640x480:
    mov $0x4F02, %ax	
    mov $0x4101, %bx /* 101 = 640x480 */
    jmp set_resolution

set_800x600:
    mov $0x4F02, %ax	
    mov $0x4103, %bx /* 103 = 800x600 */

set_resolution:
    int $0x10 
    push %es
    mov $0x4F01, %ax
    mov %bx, %cx      /* Use same resolution code for information request */	
    mov $vbe_info_structure, %di
    int $0x10
    pop %es
    ret

kill:
    movw $total_error_str, %si
    call print
    call inf 

.code32
enter32:
    /* jump to kernel loaded at 0x10000 */
    movl $vbe_info_structure, %eax
    pushl %eax
    movl $0x10000, %eax
    jmpl *%eax

inf:
    jmp inf

.code16
/* 	AL = Character, BH = Page Number, BL = Color (only in graphic mode)*/
print:
    xor %bh, %bh
    /* INT 10h needs 0x0E for teletype output */
    movb $0x0E, %ah
    lodsb

    cmpb $0, %al
    je return

    /* Check for newline character and handle it */
    cmpb $0x0A, %al
    je new_line

    int $0x10
    jmp print

new_line:
    /* Move to the start of the next line */
    /* Assuming 80 characters per line */
    mov $0x03, %ah  /* Read cursor position function */
    int $0x10       /* Get current cursor position in DX */
    inc %dh         /* Move to the next line */
    xor %dl, %dl    /* Reset column to 0 */
    mov $0x02, %ah  /* Set cursor position function */
    int $0x10       /* Set new cursor position */
    jmp print

return:
    ret

/* Strings */
welcome_str:
    .asciz "RetrOS-32\n"
total_error_str:
    .asciz "\n"
choice_str:
    .asciz "1. \n"
drive_num:
    .word 0x0000
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

/**
 * Setup descriptor for the flat memory layout
 * Giving us access to all the memory.
 * https://en.wikipedia.org/wiki/Global_Descriptor_Table
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

. = _start + 446
partition_table:
    # Partition 1 (example values, adjust as needed)
    .byte 0x80       # Bootable flag (0x80 for bootable, 0x00 for non-bootable)
    .byte 0x00       # Starting head
    .byte 0x01       # Starting sector and cylinder
    .byte 0x00       # Starting Cylinder
    .byte 0x0B       # Partition type (e.g., 0x83 for Linux)
    .byte 0x01       # Ending head
    .byte 0x20       # Ending sector 6 bit
    .byte 0x00       # Ending sector 10bit
    .byte 0x00       # Relativ sector 32bit (4 bytes)
    .byte 0x00
    .byte 0x00
    .byte 0x00
    .byte 0x00       # Total sectors 32bit (4 bytes)
    .byte 0x00
    .byte 0x00
    .byte 0x00

/* BOOT SIGNATURE */
vbe_info_structure:
    . = _start + 510
    .byte 0x55
    .byte 0xaa