#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <syscalls.h>
#include <arch/io.h>

#define ISR_LINES	49
#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_DATA	(PIC1+1)
#define PIC2_DATA	(PIC2+1)

#define IDT_ENTRIES 256

/* Code inspiried from http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */

struct registers
{
    uint32_t ds;                  /*  Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /*  Pushed by pusha. */
    uint32_t int_no, err_code;    /*  Interrupt number and error code (if applicable) */
    uint32_t eip, cs, eflags, useresp, ss; /*  Pushed by the processor automatically. */
};


void init_interrupts();
void load_data_segments(int seg);
void _syscall_entry(void);
void page_fault_interrupt(unsigned long cr2, unsigned long err);

typedef void (*isr_t)();

/*
    A struct describing an interrupt gate.
*/
struct idt_entry
{
   uint16_t base_lo;             /*  The lower 16 bits of the address to jump to when this interrupt fires. */
   uint16_t sel;                 /*  Kernel segment selector. */
   uint8_t  always0;             /*  This must always be zero. */
   uint8_t  flags;               /*  More flags. See documentation. */
   uint16_t base_hi;             /*  The upper 16 bits of the address to jump to. */
} __attribute__((packed));

struct idt_ptr
{
   uint16_t limit;
   uint32_t base;                /*  The address of the first element in our idt_entry_t array. */
} __attribute__((packed));


#define EOI(irq) \
	if (irq >= 0x28) \
		outportb(PIC2, 0x20); /* Slave */\
	outportb(PIC1, 0x20); /* Master */

extern void isr0(struct registers*);
extern void isr1(struct registers*);
extern void isr2(struct registers*);
extern void isr3(struct registers*);
extern void isr4(struct registers*);
extern void isr5(struct registers*);
extern void isr6(struct registers*);
extern void isr7(struct registers*);
extern void isr8(struct registers*);
extern void isr9(struct registers*);
extern void isr10(struct registers*);
extern void isr11(struct registers*);
extern void isr12(struct registers*);
extern void isr13(struct registers*);
extern void isr14(struct registers*);
extern void isr15(struct registers*);
extern void isr16(struct registers*);
extern void isr17(struct registers*);
extern void isr18(struct registers*);
extern void isr19(struct registers*);
extern void isr20(struct registers*);
extern void isr21(struct registers*);
extern void isr22(struct registers*);
extern void isr23(struct registers*);
extern void isr24(struct registers*);
extern void isr25(struct registers*);
extern void isr26(struct registers*);
extern void isr27(struct registers*);
extern void isr28(struct registers*);
extern void isr29(struct registers*);
extern void isr30(struct registers*);
extern void isr31(struct registers*);
extern void isr32(struct registers*);
extern void isr33(struct registers*);
extern void isr34(struct registers*);
extern void isr35(struct registers*);
extern void isr36(struct registers*);
extern void isr37(struct registers*);
extern void isr38(struct registers*);
extern void isr39(struct registers*);
extern void isr40(struct registers*);
extern void isr41(struct registers*);
extern void isr42(struct registers*);
extern void isr43(struct registers*);
extern void isr44(struct registers*);
extern void isr45(struct registers*);
extern void isr46(struct registers*);
extern void isr47(struct registers*);

int system_call(int index, int arg1, int arg2, int arg3);
void _page_fault_entry(void);

int interrupt_get_count(int interrupt);

void isr_handler(struct registers regs);
void interrupt_install_handler(int i, void (*handler)());
void idt_flush(uint32_t idt);


#endif // !INTERRUPTS_H
