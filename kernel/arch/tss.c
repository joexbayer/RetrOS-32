#include <arch/tss.h>
#include <arch/gdt.h>
#include <util.h>

void init_tss(void)
{
    uint16_t tss_p = KERNEL_TSS;

    tss.esp_0 = 0;              /* Set in setup_current_running() in */
    tss.ss_0 = 0;               /*  scheduler.c */
    tss.ldt_selector = 0;       /* No LDT, all process use same segments */
    tss.backlink = KERNEL_TSS;  /* Use the same TSS on interrupts */
    /*
     * 16-bit offset from the base of the TSS to the I/O
     * permission bitmap and interrupt redirection bitmap (which
     * is not used)
     */
    tss.iomap_base = sizeof(struct tss_entry);

    /* The rest of the fields are not used */

    /* Load the index in the GDT corresponding to the TSS
     * into the Task Register (p 153 PMSA).
     */
    asm volatile ("ltr %0" : : "m" (tss_p));
}