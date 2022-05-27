#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <util.h>

/* Code inspiried from http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */

struct registers
{
    uint32_t ds;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

void init_interrupts();


typedef void (*isr_t)();

/*
    A struct describing an interrupt gate.
*/
struct idt_entry
{
   uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   uint16_t sel;                 // Kernel segment selector.
   uint8_t  always0;             // This must always be zero.
   uint8_t  flags;               // More flags. See documentation.
   uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));

struct idt_ptr
{
   uint16_t limit;
   uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));

extern void isr0(struct registers*);
extern void isr32(struct registers*);
extern void isr33(struct registers*);
extern void isr43(struct registers*);

void isr_handler(struct registers regs);
void isr_install(size_t i, void (*handler)());
void idt_flush(uint32_t idt);
void EOI(int irq);


#endif // !INTERRUPTS_H
