#include <arch/tss.h>
#include <arch/gdt.h>
#include <libc.h>

#define FLUSH_TSS() asm volatile ("ltr %0" : : "m" (tss_selector))

struct tss_entry tss;

void init_tss(void)
{
    uint16_t tss_selector = GDT_KERNEL_TSS;
    tss.ldt_selector = 0;
    tss.prev_tss = GDT_KERNEL_TSS;
    tss.iomap_base = sizeof(struct tss_entry);

    FLUSH_TSS();
}