#include "interrupts.h"
#include "terminal.h"

/*
	Interrupts , followed tutorial:
	http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
*/

static void (*stubs[47])(registers_t*) = {
    isr0,
    isr1,
    isr2,
    isr3,
    isr4,
    isr5,
    isr6,
    isr7,
    isr8,
    isr9,
    isr10,
    isr11,
    isr12,
    isr13,
    isr14,
    isr15,
    isr16,
    isr17,
    isr18,
    isr19,
    isr20,
    isr21,
    isr22,
    isr23,
    isr24,
    isr25,
    isr26,
    isr27,
    isr28,
    isr29,
    isr30,
    isr31,
    isr32,
    isr33,
    isr34,
    isr35,
    isr36,
    isr37,
    isr38,
    isr39,
    isr40,
    isr41,
    isr42,
    isr43,
    isr44,
    isr45,
    isr46,
    isr47,
};

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

void isr_handler(registers_t regs)
{
    char test[1000];
    terminal_writestring("recieved interrupt\n");
    itoa(regs.int_no, test);
	terminal_writestring(test);
    terminal_write("\n", 1);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   idt_entries[num].flags   = flags;
}

static void init_idt()
{
   idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
   idt_ptr.base  = (uint32_t)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

    for (size_t i = 0; i < 47; i++)
    {
        idt_set_gate(i, (uint32_t)stubs[i] , 0x08, 0x8E);
    }

   idt_flush((uint32_t)&idt_ptr);
}

void init_interrupts()
{
  init_idt();
}

