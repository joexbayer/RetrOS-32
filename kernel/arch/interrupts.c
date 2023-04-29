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
#include <arch/interrupts.h>
#include <arch/gdt.h>
#include <pcb.h>
#include <serial.h>
#include <scheduler.h>
#include <kthreads.h>
#include <assert.h>
#include <kutils.h>

//TEMP
#include <memory.h>

static struct idt_entry idt_entries[IDT_ENTRIES];
static struct idt_ptr   idt;
/* Handlers, default 0, will be installed when needed. */
static void (*handlers[ISR_LINES])() = { 0 };
static void (*irqs[ISR_LINES])(struct registers*) = {
	isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,
	isr12,isr13,isr14,isr15,isr16,isr17,isr18,isr19,isr20,isr21,
	isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31,
	isr32,isr33,isr34,isr35,isr36,isr37,isr38,isr39,isr40,isr41,
	isr42,isr43,isr44,isr45,isr46,isr47,
};

static const char* __exceptions_names[32] = {
	"Divide by zero","Debug","NMI","Breakpoint","Overflow",
	"OOB","Invalid opcode","No coprocessor","Double fault",
	"Coprocessor segment overrun","Bad TSS","Segment not present",
	"Stack fault","General protection fault","Page fault",
	"Unrecognized interrupt","Coprocessor fault","Alignment check",
	"Machine check","RESERVED","RESERVED","RESERVED","RESERVED",
	"RESERVED","RESERVED","RESERVED","RESERVED","RESERVED","RESERVED",
	"RESERVED"
};

void page_fault_interrupt(unsigned long cr2, unsigned long err)
{
	CLI();
	dbgprintf("Page fault: 0x%x (Stack: 0x%x) %d (%s)\n", cr2, current_running->stack_ptr, err, current_running->name);
	dbgprintf("Page: %x, process: %s\n", current_running->page_dir[DIRECTORY_INDEX(cr2)], current_running->name);
	pcb_dbg_print(current_running);
	kernel_exit();
}

void general_protection_fault()
{
	CLI();
	dbgprintf("General Protection Fault: 0x%x - %s\n", current_running->stack_ptr, current_running->name);
	pcb_dbg_print(current_running);

	/* TODO: Some kind of feedback */
	EOI(13);
	kernel_exit();
	UNREACHABLE();
}
/**
 * @brief Given a IRQ line, it assigns a handler to it. 
 * 
 * @param i IRQ line
 * @param handler function pointer to handler.
 */
void interrupt_install_handler(int i, void (*handler)())
{
	handlers[i] = handler;
}

static void __interrupt_exception_handler(int i)
{
	dbgprintf("[exception] %d %s (%s)\n", i, __exceptions_names[i], current_running->name);
	pcb_dbg_print(current_running);\
	PANIC();
}

void load_data_segments(int seg)
{
    asm volatile ("movw %%ax, %%ds \n\t" "movw %% ax, %%es " : : "a" (seg));
}

/* Main interrupt handler, calls interrupt specific hanlder if installed. */
void isr_handler(struct registers regs)
{	
	if(regs.int_no < 32){
		__interrupt_exception_handler(regs.int_no);
		EOI(regs.int_no);
		return;
	}

	if (handlers[regs.int_no] != 0){
		isr_t handler = handlers[regs.int_no];
		handler();
	}

	if(regs.int_no != 32)
		EOI(regs.int_no);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t type, uint8_t access)
{
	idt_entries[num].base_lo = base & 0xFFFF; /* Low part of address */
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF; /* High part of address */
	idt_entries[num].sel     = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags   = type | access << 5 | 1 << 7;
}

/* TODO: Setup exceptions. */

static void init_idt()
{
	idt.limit = sizeof(struct idt_entry) * 256 -1;
	idt.base  = (uint32_t)&idt_entries;

	memset(&idt_entries, 0, sizeof(struct idt_entry)*256);

	/* Set all ISR_LINES to go to ISR0 */
	for (int i = 0; i < 48; i++){ 
		idt_set_gate(i, (uint32_t) irqs[i] , GDT_KERNEL_CS, 0x0E, 0);
	}
	
	idt_set_gate(48, (uint32_t)&_syscall_entry, GDT_KERNEL_CS, 0x0E, 3);
	idt_set_gate(14, (uint32_t)&_page_fault_entry, GDT_KERNEL_CS, 0x0E, 0);

	interrupt_install_handler(13, &general_protection_fault);

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

	dbgprintf("[IQR] Interrupts initialized.");
}

