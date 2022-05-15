#include "interrupts.h"
#include "terminal.h"

#define PIT_IRQ 32
/*
	Interrupts , followed tutorial:
	http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
*/

static void (*stubs[47])(registers_t*) = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9,
    isr10, isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18,
    isr19, isr20, isr21, isr22, isr23, isr24, isr25, isr26, isr27,
    isr28, isr29, isr30, isr31, isr32, isr33, isr34, isr35, isr36,
    isr37, isr38, isr39, isr40, isr41, isr42, isr43, isr44, isr45,
    isr46, isr47,
};

static void (*handlers[47])(registers_t*) = { 0 };
void isr_install(size_t i, void (*handler)(registers_t*)) {
    handlers[i] = handler;
}

idt_entry_t idt_entries[256];
idt_ptr_t   idt;

void isr_handler(registers_t regs)
{
    outb(0xA0, 0x20);
    outb(0x20, 0x20);

    if (handlers[regs.int_no] != 0)
    {
        isr_t handler = handlers[regs.int_no];
        handler(regs);
    }
    char test[1000];
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   idt_entries[num].flags   = flags;
}

int tick = 0;
static void timer_callback(registers_t regs)
{
   tick++;
   char test[1000];
   itoa(tick % 1000, test);
   terminal_write_position(10, 10, test);
}

void init_timer(uint32_t frequency)
{
   // Firstly, register our timer callback.
   isr_install(PIT_IRQ, &timer_callback);

   // The value we send to the PIT is the value to divide it's input clock
   // (1193180 Hz) by, to get our required frequency. Important to note is
   // that the divisor must be small enough to fit into 16-bits.
   uint32_t divisor = 1193180 / frequency;

   // Send the command byte.
   outb(0x43, 0x36);

   // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
   uint8_t l = (uint8_t)(divisor & 0xFF);
   uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

   // Send the frequency divisor.
   outb(0x40, l);
   outb(0x40, h);

   terminal_writestring("Timer Started.\n");
}

static void init_idt()
{
   idt.limit = sizeof(idt_entry_t) * 256 -1;
   idt.base  = (uint32_t)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

    for (size_t i = 0; i < 47; i++)
    {
        idt_set_gate(i, (uint32_t)stubs[i] , 0x08, 0x8E);
    }

   idt_flush((uint32_t)&idt);
}

void init_interrupts()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    init_idt();
    init_timer(1);
    STI();
}

