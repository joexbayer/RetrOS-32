#include <arch/gdt.h>
#include <arch/tss.h>

#define KERNEL_PRIVILEGE 0
#define PROCESSS_PRIVILEGE 3
#define FLUSH_GDT() asm volatile ("lgdt %0" : : "m" (gdt_addr))

static struct gdt_segment gdt[7];
struct tss_entry tss;

static struct gdt_address
{
   uint16_t limit;
   uint32_t base;
} __attribute__ ((packed)) gdt_addr = {
   .limit = 7 * 8 - 1
};

void gdt_set_segment(struct gdt_segment *segment, uint32_t base, uint32_t limit, char type, char privilege, char system)
{
   segment->limit_low = (uint16_t)limit;
   segment->limit_high = (uint8_t)(limit >> 16);
   segment->base_low = (uint16_t)base;
   segment->base_mid = (uint8_t)(base >> 16);
   segment->base_high = (uint8_t)(base >> 24);
   /* Byte 5 [0:3] = type
   * Byte 5 [4]   = system
   * Byte 5 [5:6] = privilege level
   * Byte 5 [7]   = 1 (always present) */
   segment->access = type | system << 4 | privilege << 5 | 1 << 7;
   /* Byte 6 [0:3] = limit_high
   * Byte 6 [4]   = 0 (not avilable for use by system software)
   * Byte 6 [5]   = 0 (reserved)
   * Byte 6 [6]   = 1 (D/B bit)
   * Byte 6 [7]   = 1 (length of limit is in pages) */
   segment->limit_high |= 1 << 7 | 1 << 6;
}


void init_gdt()
{
   gdt_addr.base = (uint32_t) gdt;

   gdt_set_segment(gdt + KERNEL_CODE, 0, 0xfffff, CODE_SEGMENT, KERNEL_PRIVILEGE, MEMORY);

    /* Data segment for the kernel (contains kernel stack) */
    gdt_set_segment(gdt + KERNEL_DATA, 0, 0xfffff, DATA_SEGMENT, KERNEL_PRIVILEGE, MEMORY);

    /* Code segment for processes */
    gdt_set_segment(gdt + PROCESS_CODE, 0, 0xfffff, CODE_SEGMENT, PROCESSS_PRIVILEGE, MEMORY);

    /* Data segment for processes (contains user stack) */
    gdt_set_segment(gdt + PROCESS_DATA, 0, 0xfffff, DATA_SEGMENT, PROCESSS_PRIVILEGE, MEMORY);

    /* Insert pointer to the global TSS */
    gdt_set_segment(gdt + TSS_INDEX, (uint32_t)&tss, TSS_SIZE, TSS_SEGMENT, KERNEL_PRIVILEGE, SYSTEM); /* is a system segment */

   FLUSH_GDT();

       /* Reload the Segment registers to refresh the hidden portions */
    asm volatile ("pushl %ds");
    asm volatile ("popl %ds");

    asm volatile ("pushl %es");
    asm volatile ("popl %es");

    asm volatile ("pushl %ss");
    asm volatile ("popl %ss");
}
