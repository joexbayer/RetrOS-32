#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "util.h"

/* Code inspiried from http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */

typedef struct registers
{
    uint32_t ds;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

void init_interrupts();


typedef void (*isr_t)(registers_t);

/*
    A struct describing an interrupt gate.
*/
struct idt_entry_struct
{
   uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   uint16_t sel;                 // Kernel segment selector.
   uint8_t  always0;             // This must always be zero.
   uint8_t  flags;               // More flags. See documentation.
   uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

struct idt_ptr_struct
{
   uint16_t limit;
   uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

extern void isr0(registers_t*);
extern void isr1(registers_t*);
extern void isr2(registers_t*);
extern void isr3(registers_t*);
extern void isr4(registers_t*);
extern void isr5(registers_t*);
extern void isr6(registers_t*);
extern void isr7(registers_t*);
extern void isr8(registers_t*);
extern void isr9(registers_t*);
extern void isr10(registers_t*);
extern void isr11(registers_t*);
extern void isr12(registers_t*);
extern void isr13(registers_t*);
extern void isr14(registers_t*);
extern void isr15(registers_t*);
extern void isr16(registers_t*);
extern void isr17(registers_t*);
extern void isr18(registers_t*);
extern void isr19(registers_t*);
extern void isr20(registers_t*);
extern void isr21(registers_t*);
extern void isr22(registers_t*);
extern void isr23(registers_t*);
extern void isr24(registers_t*);
extern void isr25(registers_t*);
extern void isr26(registers_t*);
extern void isr27(registers_t*);
extern void isr28(registers_t*);
extern void isr29(registers_t*);
extern void isr30(registers_t*);
extern void isr31(registers_t*);
extern void isr32(registers_t*);
extern void isr33(registers_t*);
extern void isr34(registers_t*);
extern void isr35(registers_t*);
extern void isr36(registers_t*);
extern void isr37(registers_t*);
extern void isr38(registers_t*);
extern void isr39(registers_t*);
extern void isr40(registers_t*);
extern void isr41(registers_t*);
extern void isr42(registers_t*);
extern void isr43(registers_t*);
extern void isr44(registers_t*);
extern void isr45(registers_t*);
extern void isr46(registers_t*);
extern void isr47(registers_t*);

void isr_handler(registers_t regs);


#endif // !INTERRUPTS_H
