#include <arch/gdt.h>
#include <arch/tss.h>

struct gdt_ptr {
   uint16_t limit;               /*  The upper 16 bits of all selector limits. */
   uint32_t base;                /*  The address of the first gdt_entry_t struct. */
}__attribute__((packed));


struct gdt_entry gdt_entries[5];
struct gdt_ptr   gdt_ptr;


void create_segment(struct segment *entry,    /* GDT entry */
                    uint32_t base,              /* segment start address */
                    uint32_t limit,             /* segment size */
                    char type,                  /* segment type (OS system-,
                                                   code-, data-, stack segment) */
                    char privilege,             /* descriptor privilege level */
                    char system /* system bit (0: system segm.) */)
{
    entry->limit_low = (uint16_t)limit;
    entry->limit_high = (uint8_t)(limit >> 16);
    entry->base_low = (uint16_t)base;
    entry->base_mid = (uint8_t)(base >> 16);
    entry->base_high = (uint8_t)(base >> 24);
    /* Byte 5 [0:3] = type
     * Byte 5 [4]   = system
     * Byte 5 [5:6] = privilege level
     * Byte 5 [7]   = 1 (always present) */
    entry->access = type | system << 4 | privilege << 5 | 1 << 7;
    /* Byte 6 [0:3] = limit_high
     * Byte 6 [4]   = 0 (not avilable for use by system software)
     * Byte 6 [5]   = 0 (reserved)
     * Byte 6 [6]   = 1 (D/B bit)
     * Byte 6 [7]   = 1 (length of limit is in pages) */
    entry->limit_high |= 1 << 7 | 1 << 6;
}

/*  Set the value of one GDT entry. */
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
   gdt_entries[num].base_low    = (base & 0xFFFF);
   gdt_entries[num].base_middle = (base >> 16) & 0xFF;
   gdt_entries[num].base_high   = (base >> 24) & 0xFF;

   gdt_entries[num].limit_low   = (limit & 0xFFFF);
   gdt_entries[num].granularity = (limit >> 16) & 0x0F;

   gdt_entries[num].granularity |= gran & 0xF0;
   gdt_entries[num].access      = access;
} 

static struct segment gdt[7];
struct tss_entry tss;

void init_gdt()
{
   struct point gdt_p;
   create_segment(
        gdt + KERNEL_CODE,      /* gdt entry */
        0,                      /* memory start address */
        0xfffff,                /* size (4 GB) */
        CODE_SEGMENT,           /* type = code segment */
        0,                      /* highest privilege level */
        MEMORY);                /* is not a system segment */

    /* Data segment for the kernel (contains kernel stack) */
    create_segment(gdt + KERNEL_DATA, 0, 0xfffff, DATA_SEGMENT,
                   0, /* highest privilege level */
                   MEMORY);

    /* Code segment for processes */
    create_segment(gdt + PROCESS_CODE, 0, 0xfffff, CODE_SEGMENT,
                   3, /* lowest privilege level */
                   MEMORY);

    /* Data segment for processes (contains user stack) */
    create_segment(gdt + PROCESS_DATA, 0, 0xfffff, DATA_SEGMENT,
                   3, /* lowest privilege level */
                   MEMORY);

    /* Insert pointer to the global TSS */
    create_segment(gdt + TSS_INDEX, (uint32_t)&tss,
                   TSS_SIZE, TSS_SEGMENT,
                   0,       /* highest privilege level */
                   SYSTEM); /* is a system segment */

    /*
     * Load the GDTR register with a pointer to the gdt, and the
     * size of the gdt
     */
    gdt_p.limit = 7 * 8 - 1;
    gdt_p.base = (uint32_t)gdt;

    asm volatile ("lgdt %0" : : "m" (gdt_p));

    /* Reload the Segment registers to refresh the hidden portions */
    asm volatile ("pushl %ds");
    asm volatile ("popl %ds");

    asm volatile ("pushl %es");
    asm volatile ("popl %es");

    asm volatile ("pushl %ss");
    asm volatile ("popl %ss");

   //gdt_flush((uint32_t)&gdt_ptr);
   //tss_flush();
}
