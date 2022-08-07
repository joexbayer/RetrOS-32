/**
 * @file interrupts.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles installing, calling and handling of interrupts 
 * @see http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <interrupts.h>
#include <terminal.h>
#include <screen.h>
#include <io.h>

static struct idt_entry idt_entries[IDT_ENTRIES];
static struct idt_ptr   idt;

/* TODO: Move to own file? */
typedef int (*syscall_t) ();
syscall_t syscall[10];

int invoke_syscall(int i, int arg1, int arg2, int arg3)
{
    int ret;

    asm volatile ("int $48" /* 48 = 0x30 */
                  : "=a" (ret)
                  : "%0" (i), "b" (arg1), "c" (arg2), "d" (arg3));
    return ret;
}

void add_system_call(int index, syscall_t fn)
{
	syscall[index] = fn;
}

int system_call(int index, int arg1, int arg2, int arg3)
{	
	twritef("%d %d %d %d\n", index, arg1, arg2,arg3);
	EOI(48);
	return 99;

	if(index < 0)
		return -1;

	/* Call system call function based on index. */
	syscall_t fn = syscall[index];
	int ret = fn(arg1, arg2, arg3);

	return ret;
}

int page_fault_interrupt(unsigned long cr2, unsigned long err)
{
	CLI();
	scrprintf(10, 9, "CR2: %x %d   ERR: %d", cr2 &  0xfffff000, cr2, err);
	scrprintf(10, 10, "EXCEPTION 14!!!");
	while(1);
	
}


/* Handlers, default 0, will be installed when needed. */
static void (*handlers[ISR_LINES])() = { 0 };

static void (*irqs[ISR_LINES])(struct registers*) = {
	isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
	isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
};
/**
 * @brief Given a IRQ line, it assigns a handler to it. 
 * 
 * @param i IRQ line
 * @param handler function pointer to handler.
 */
void isr_install(size_t i, void (*handler)()) {
	handlers[i] = handler;
}

void EOI(int irq)
{
	if (irq >= 0x28)
		outportb(PIC2, 0x20); /* Slave */	
	
	outportb(PIC1, 0x20); /* Master */
}

/* Main interrupt handler, calls interrupt specific hanlder if installed. */
void isr_handler(struct registers regs)
{
	if (handlers[regs.int_no] != 0)
	{
		isr_t handler = handlers[regs.int_no];
		handler();
	}
	EOI(regs.int_no);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
	idt_entries[num].base_lo = base & 0xFFFF; /* Low part of address */
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF; /* High part of address */
	idt_entries[num].sel     = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags   = flags;
}

/* TODO: Setup exceptions. */

static void init_idt()
{
	idt.limit = sizeof(struct idt_entry) * 256 -1;
	idt.base  = (uint32_t)&idt_entries;

	memset(&idt_entries, 0, sizeof(struct idt_entry)*256);

	/* Set all ISR_LINES to go to ISR0 */
	for (size_t i = 0; i < 32; i++)
	{ 
		idt_set_gate(i, (uint32_t) isr0 , 0x08, 0x8E);
	}

	for (size_t i = 32; i < 48; i++)
	{
		idt_set_gate(i, (uint32_t) irqs[i-32] , 0x08, 0x8E); // PIT timer
	}
	idt_set_gate(48, (uint32_t)&_syscall_entry, 0x08, 0x8E);
	//idt_set_gate(14, &_page_fault_entry, 0x08, 0x8E);

	idt_flush((uint32_t)&idt);
}

void init_interrupts()
{
	/* Remap PIC to correctly use IRQ's */
	outportb(PIC1, 0x11);
	outportb(PIC2, 0x11);
	outportb(PIC1_DATA, 0x20);
	outportb(PIC2_DATA, 0x28);
	outportb(PIC1_DATA, 0x04);
	outportb(PIC2_DATA, 0x02);
	outportb(PIC1_DATA, 0x01);
	outportb(PIC2_DATA, 0x01);
	outportb(PIC1_DATA, 0x0);
	outportb(PIC2_DATA, 0x0);

	init_idt();

	twriteln("Interrupts initialized.");
}

