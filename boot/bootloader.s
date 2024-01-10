/**
 * @file Bootloader.s 
 * @author Joe Bayer (joexbayer)
 * @brief Main bootloader for RetrOS-32 
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

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

    call reset_screen
    /* Greet user with message. */
    movw $welcome_str, %si
    call print

/* load second stage bootloader */
load_stage2:
    /* Setup ES:BX to point to 0x7E00 for the second stage */
    mov $0x0000, %ax
    mov %ax, %es
    mov $0x7E00, %bx

    /* Setup and call BIOS interrupt 0x13 to read sectors for the second stage */
    mov $0x02, %ah          /* Function number for read sectors */
    mov $0x03, %al          /* Number of sectors to read */
    mov $0x00, %ch          /* Cylinder number */
    mov $0x02, %cl          /* Sector number (starting at 2, right after the boot sector) */
    mov $0x00, %dh          /* Head number */
    mov drive_num, %dl      /* Drive number (0x00 for floppy, 0x80 for hard disk) */

    int $0x13               /* Call interrupt 13h */
    jc read_error           /* Jump to error handling if carry flag is set */
    /* Far jump to the second stage bootloader */
    jmp _stage2_init

reset_screen:
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

    ret

read_error:
    movw $welcome_err_str, %si
    call print
kill:
    movw $total_error_str, %si
    call print
    call inf 
inf:
    hlt
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

/* Stage 1 Strings */
welcome_str:
    .asciz "RetrOS-32 - Stage 1\n====================\n"
welcome_err_str:
    .asciz " > Error reading from disk.\n"
total_error_str:
    .asciz "\nUnable to load stage 2 bootloader\n"
drive_num:
    .word 0x0000

# Boot partition set.
. = _start + 446
partition_table:
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
bootsig:
    . = _start + 510
    .byte 0x55
    .byte 0xaa

/**
 *
 * Stage 2
 * Loads the kernel and sets up the GDT 
 * Configures video mode and enables A20 line
 */

.org 0x0200
boot_info:
    extended_memory_low: .long 0  
    extended_memory_high: .long 0
    textmode: .long 0
_stage2_init:
    jmp _stage2

/* Stage 2 strings */
/* Box drawing characters with left padding for centering */
top_left_corner:
    .asciz "                     \xDA"
top_right_corner:
    .asciz "\xBF\n"
bottom_left_corner:
    .asciz "                     \xC0"
bottom_right_corner:
    .asciz "\xD9\n"
straight_line:
    .asciz "                     \xB3"
straight_line_end:
    .asciz "\xB3\n"
left_cross:
    .asciz "                     \xC3"
right_cross_end:
    .asciz "\xB4\n"
separator:
    .asciz "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4"


/* Adjusted strings with padding to match the length of the separator */
home_str:
    .asciz "                       Welcome to RetrOS-32 Bootloader  \n"
kernel_str:
    .asciz "Loaded kernel.                   "
choice_str:
    .asciz "Choose resolution:               "
choice_1_str:
    .asciz "   1. 640x480                    "
choice_2_str:
    .asciz "   2. 800x600                    "
choice_3_str:
    .asciz "   3. 1024x768                   "
choice_4_str:
    .asciz "   ESC. Textmode                 "
memory_error_str:
    .asciz "Memory detection: ERROR          "
mem_str:
    .asciz "Memory detection: OK             "
new_line_str:
    .asciz "\n"

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

/* Stage 2 code */

draw_middle_separator:
    movw $left_cross, %si
    call print
    movw $separator, %si
    call print
    movw $right_cross_end, %si
    call print

    ret

draw_top_separator:
    movw $top_left_corner, %si
    call print
    movw $separator, %si
    call print
    movw $top_right_corner, %si
    call print

    ret

draw_bottom_separator:
    movw $bottom_left_corner, %si
    call print
    movw $separator, %si
    call print
    movw $bottom_right_corner, %si
    call print

    ret

print_main_window:
    call reset_screen

    movw $home_str, %si
    call print

    call draw_top_separator

    /* Print kernel status */
    movw $straight_line, %si
    call print
    movw $kernel_str, %si
    call print
    movw $straight_line_end, %si
    call print
    /* Print the middle separator */
    call draw_middle_separator


    ret

_stage2:
    call print_main_window

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
    movw $4, sector

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

    call detect_memory_size

    call set_video_mode

    /* enable A20 line */
    call set_a20
    /* enable the PE flag */
    movl %cr0, %eax
    orl $0x1, %eax
    movl %eax, %cr0

/* Setup GDT for 32 land */

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
    # je set_640x480

    /* Print resolution choice */
    movw $straight_line, %si
    call print
    movw $choice_str, %si
    call print
    movw $straight_line_end, %si
    call print

    movw $straight_line, %si
    call print
    movw $choice_1_str, %si
    call print
    movw $straight_line_end, %si
    call print

    movw $straight_line, %si
    call print
    movw $choice_2_str, %si
    call print
    movw $straight_line_end, %si
    call print

    movw $straight_line, %si
    call print
    movw $choice_3_str, %si
    call print
    movw $straight_line_end, %si
    call print

    movw $straight_line, %si
    call print
    movw $choice_4_str, %si
    call print
    movw $straight_line_end, %si
    call print

    call draw_bottom_separator

    /* Print message asking for resolution choice */
    /* Wait for key press */
    mov $0x00, %ah     /* Function 0x00 - Get Keypress */
    int $0x16          /* BIOS Keyboard Services */

    /* Check if pressed key is '1' or '2' */
    cmp $'1', %al
    je set_640x480
    cmp $'2', %al
    je set_800x600
    cmp $'3', %al
    je set_1024x768
    cmp $0x1B, %al
    je set_textmode

    jmp set_video_mode

set_640x480:
    mov $0x4F02, %ax	
    mov $0x4101, %bx /* 101 = 640x480 */
    jmp set_resolution

set_800x600:
    mov $0x4F02, %ax	
    mov $0x4103, %bx /* 103 = 800x600 */
    jmp set_resolution

set_1024x768:
    mov $0x4F02, %ax	
    mov $0x4105, %bx /* 105 = 1024x768 */
    jmp set_resolution

set_textmode:
    mov $1, %AX
    mov %ax, 0x7E08 /* Set textmode flag */
    ret

set_resolution:
    int $0x10 
    push %es
    mov $0x4F01, %ax
    mov %bx, %cx      /* Use same resolution code for information request */	
    mov $vbe_info_structure, %di
    int $0x10
    pop %es
    ret

detect_memory_size:
    xor %cx, %cx
    xor %dx, %dx

    mov $0xE801, %ax
    int $0x15
    jc mem_error

    cmp $0x86, %ah
    je mem_error

    cmp $0x80, %ah
    je mem_error

    jcxz use_ax

    mov %cx, %ax
    mov %dx, %bx

    mov %ax, 0x7E00
    mov %bx, 0x7E04

/* AX/BX already contain the required values */
use_ax:
print_memory_size:
    movw $straight_line, %si
    call print
    movw $mem_str, %si
    call print
    movw $straight_line_end, %si
    call print
    ret
    
mem_error:
    /* Handle error */
    movw $straight_line, %si
    call print
    movw $memory_error_str, %si
    call print
    movw $straight_line_end, %si
    call print
    ret

.code32
enter32:
    /* jump to kernel loaded at 0x10000 */
    movl $vbe_info_structure, %eax
    pushl %eax
    movl $0x10000, %eax
    jmpl *%eax

.code16
vbe_info_structure:
    . = _start + 510 + 1024 + 512
    .byte 0x55
    .byte 0xaa