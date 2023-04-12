#include <arch/gdt.h>
#include <arch/tss.h>

struct gdt_ptr
{
   uint16_t limit;               /*  The upper 16 bits of all selector limits. */
   uint32_t base;                /*  The address of the first gdt_entry_t struct. */
}
 __attribute__((packed));

 extern void gdt_flush(uint32_t);


struct gdt_entry gdt_entries[5];
struct gdt_ptr   gdt_ptr;


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

void init_gdt()
{
   gdt_ptr.limit = (sizeof(struct gdt_ptr) * 6) - 1;
   gdt_ptr.base  = (uint32_t)&gdt_entries;

   gdt_set_gate(0, 0, 0, 0, 0);                /*  Null segment */
   gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /*  Code segment */
   gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /*  Data segment */
   gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /*  User mode code segment */
   gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /*  User mode data segment */
   write_tss(5, 0x10, 0x0);

   gdt_flush((uint32_t)&gdt_ptr);
   tss_flush();
}
