.code32
.section .text.prologue

.global tlb_flush_addr
.text
tlb_flush_addr:
  movl 4(%esp), %eax
  invlpg (%eax)
  ret

.text
.globl load_page_directory
load_page_directory:
    push %ebp
    mov %esp, %ebp
    pushl %eax

    mov 12(%esp), %eax
    mov %eax, %cr3

    popl %eax
    mov %ebp, %esp
    pop %ebp
    ret

.global gdt_flush
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ret

.global tss_flush
tss_flush:
   mov $0x2B, %ax
   ltr %ax
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

.global _context_switch
_context_switch:
    cli
    addl $1, cli_cnt
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
    subl $1, cli_cnt
    ret

.global _start_pcb
_start_pcb:
    movl current_running, %eax
    movl 4(%eax), %esp
    movl 0(%eax), %ebp
    movl 152(%eax), %ebx
    movl 156(%eax), %ecx
    movl 8(%eax), %edx
    subl $1, cli_cnt

    /*pushl 8(%eax)
    sti
    subl $1, cli_cnt
    # jmp *8(%eax)
    ret */

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
                
    mov %esp, %eax
    pushl $0x10
    pushl %eax
    pushf
    pop %eax
    or $0x200, %eax
    push %eax 
    pushl $0x08

    pushl %edx
    iret

.section .text
.align 4
