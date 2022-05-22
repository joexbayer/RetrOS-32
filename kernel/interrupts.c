#include <interrupts.h>
#include <terminal.h>
#include <screen.h>

#define ISR_LINES	48
#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_DATA	(PIC1+1)
#define PIC2_DATA	(PIC2+1)

idt_entry_t idt_entries[256];
idt_ptr_t   idt;

/*
	Interrupts , followed tutorial:
	http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
*/


/* Handlers, default 0, will be installed when needed. */
static void (*handlers[ISR_LINES])() = { 0 };

void isr_install(size_t i, void (*handler)()) {
	handlers[i] = handler;
}

void EOI(int irq)
{
	if (irq >= 0x28)
	{
		outportb(PIC2, 0x20); /* Slave */	
	}
	outportb(PIC1, 0x20); /* Master */
}

/* Main interrupt handler, calls interrupt specific hanlder if installed. */
void isr_handler(registers_t regs)
{	
	if(regs.int_no != 0)
		scrprintf(12, 12, "IRQ: %d", regs.int_no);

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

static void init_idt()
{
	idt.limit = sizeof(idt_entry_t) * 256 -1;
	idt.base  = (uint32_t)&idt_entries;

	memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

	/* Set all ISR_LINES to go to ISR0 */
	for (size_t i = 0; i < ISR_LINES; i++)
	{ 
		idt_set_gate(i, (uint32_t) isr0 , 0x08, 0x8E);
	}

	/* Override to use correct entry point*/
	idt_set_gate(32, (uint32_t) isr32 , 0x08, 0x8E); // PIT timer
	idt_set_gate(33, (uint32_t) isr33 , 0x08, 0x8E); // Keyboard
	idt_set_gate(43, (uint32_t) isr43 , 0x08, 0x8E); // e1000

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

	twrite("Interrupts initialized.\n");
}

