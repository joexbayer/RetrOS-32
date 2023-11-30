#include <arch/gdt.h>

.equ PCB_CTX, 0  /* pcb_state struct start offset */
.equ PCB_EAX, PCB_CTX + 0
.equ PCB_ECX, PCB_CTX + 4
.equ PCB_EDX, PCB_CTX + 8
.equ PCB_EBX, PCB_CTX + 12
.equ PCB_ESP, PCB_CTX + 16
.equ PCB_EBP, PCB_CTX + 20
.equ PCB_ESI, PCB_CTX + 24
.equ PCB_EDI, PCB_CTX + 28
.equ PCB_EIP, PCB_CTX + 32
.equ PCB_EFLAGS, PCB_CTX + 36
.equ PCB_FPU_STATE, PCB_CTX + 40

.equ PCB_ARGS, PCB_CTX + 148
.equ PCB_ARGV, PCB_CTX + 152
.equ PCB_DS, PCB_CTX + 156
.equ PCB_CS, PCB_CTX + 160
.equ PCB_KESP, PCB_CTX + 164
.equ PCB_KEBP, PCB_CTX + 168
.equ PCB_IS_PROCESS, PCB_CTX + 172
.equ PCB_THREAD_EIP, PCB_CTX + 173


NEW_PCB_INIT_EFLAGS = ((0 << 12) | (1 << 9))

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
    pushl %ebp
    movl %esp, %ebp

    pushl %eax

    mov 12(%esp), %eax
    mov %eax, %cr3

    popl %eax

    movl %ebp, %esp
    popl %ebp
    ret

.text
.globl enable_paging
enable_paging:
    pushl %ebp
    movl %esp, %ebp

    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    
    movl %ebp, %esp
    popl %ebp
    ret

.global pcb_save_context 
pcb_save_context:
    pushl %ebp
    movl %esp, %ebp

    pushl %eax

    movl 12(%esp), %eax

    pushl %ebx

    movl %ebx, PCB_EBX(%eax)
    movl %ecx, PCB_ECX(%eax)
    movl %edx, PCB_EDX(%eax)
    movl %esi, PCB_ESI(%eax)
    movl %edi, PCB_EDI(%eax)
    movl %esp, PCB_ESP(%eax)
    movl %ebp, PCB_EBP(%eax)

    pushfl
    popl %ebx
    movl %ebx, PCB_EFLAGS(%eax)
    fnsave PCB_FPU_STATE(%eax)

    popl %ebx

    popl %eax

    movl %ebp, %esp
    popl %ebp

    ret

.global pcb_restore_context 
pcb_restore_context:
    /* preserve the current context */
    pushl %ebp
    movl %esp, %ebp

    pushl %eax

    movl 12(%esp), %eax


    movl PCB_EBP(%eax), %ebp  /* ebp */
    movl PCB_ESP(%eax), %esp  /* esp */

    frstor PCB_FPU_STATE(%eax)  /* fpu_state */

    movl PCB_EBX(%eax), %ebx  /* ebx */
    movl PCB_ECX(%eax), %ecx   /* ecx */
    movl PCB_EDX(%eax), %edx   /* edx */
    movl PCB_ESI(%eax), %esi  /* esi */
    movl PCB_EDI(%eax), %edi  /* edi */

    pushl PCB_EFLAGS(%eax)  /* push EFLAGS onto the stack */
    popfl  /* pop EFLAGS from the stack */

    popl %eax

    movl %ebp, %esp
    popl %ebp

    ret

.global _start_pcb
_start_pcb:
    subl $1, cli_cnt
    # movl current_running, %eax
    movl 4(%esp), %eax

    movl PCB_KESP(%eax), %esp

    # This forwards the arguments to the new process
    # Thread entry point is stored in PCB_THREAD_EIP, 0 for process.
    movl PCB_ARGS(%eax), %ebx
    movl PCB_ARGV(%eax), %ecx
    movl PCB_THREAD_EIP(%eax), %edx
    
    cmpl $0, 20(%eax)
    je _start_pcb_skip
    pushl	PCB_DS(%eax)
    pushl	PCB_ESP(%eax)
_start_pcb_skip:
    pushl	$NEW_PCB_INIT_EFLAGS
    pushl	PCB_CS(%eax)
    pushl	PCB_EIP(%eax)

    movw	PCB_DS(%eax),%ds
    movw	PCB_DS(%eax),%es
    
    iret

.section .text
.align 4
