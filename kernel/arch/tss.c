#include <arch/tss.h>
#include <arch/gdt.h>
#include <util.h>

#define FLUSH_TSS() asm volatile ("ltr %0" : : "m" (tss_selector))

void init_tss(void)
{
    uint16_t tss_selector = KERNEL_TSS;
    tss.ldt_selector = 0;
    tss.prev_tss = KERNEL_TSS;
    tss.iomap_base = sizeof(struct tss_entry);

    FLUSH_TSS();
}