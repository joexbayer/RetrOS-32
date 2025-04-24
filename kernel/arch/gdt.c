/**
 * @file gdt.c
 * @author Joe Bayer (joexbayer)
 * @brief Global Descriptor Table
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <arch/gdt.h>
#include <arch/tss.h>

#define FLUSH_GDT() asm volatile ("lgdt %0" : : "m" (gdt_addr))

static struct gdt_segment gdt[7];
struct gdt_address
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

static struct gdt_address gdt_addr;

void gdt_set_segment(struct gdt_segment *segment, uint32_t base, uint32_t limit, char type, char privilege, char system)
{
	segment->limit_lo = (uint16_t)limit;
	segment->limit_hi = (uint8_t)(limit >> 16);
	segment->base_lo = (uint16_t)base;
	segment->base_mid = (uint8_t)(base >> 16);
	segment->base_hi = (uint8_t)(base >> 24);
	segment->access = type | system << 4 | privilege << 5 | 1 << 7;
	segment->limit_hi |= 1 << 7 | 1 << 6;
}


void init_gdt()
{
	gdt_addr.limit =  7 * 8 - 1;
	gdt_addr.base = (uint32_t) gdt;

	/* Code segment for the kernel */
	gdt_set_segment(gdt + GDT_KERNEL_CODE, 0, 0xfffff, GDT_CODE_SEGMENT, KERNEL_PRIVILEGE, MEMORY);
	/* Data segment for the kernel */
	gdt_set_segment(gdt + GDT_KERNEL_DATA, 0, 0xfffff, GDT_DATA_SEGMENT, KERNEL_PRIVILEGE, MEMORY);
	/* Code segment for processes */
	gdt_set_segment(gdt + GDT_PROCESS_CODE, 0, 0xfffff, GDT_CODE_SEGMENT, PROCESSS_PRIVILEGE, MEMORY);
	/* Data segment for processes */
	gdt_set_segment(gdt + GDT_PROCESS_DATA, 0, 0xfffff, GDT_DATA_SEGMENT, PROCESSS_PRIVILEGE, MEMORY);
	/* TSS segment */
	gdt_set_segment(gdt + GDT_TSS_INDEX, (uint32_t)&tss, TSS_SIZE, GDT_TSS_SEGMENT, KERNEL_PRIVILEGE, SYSTEM); /* is a system segment */

	FLUSH_GDT();

	/* Reload the Segment registers*/
	asm volatile ("pushl %ds");
	asm volatile ("popl %ds");

	asm volatile ("pushl %es");
	asm volatile ("popl %es");

	asm volatile ("pushl %ss");
	asm volatile ("popl %ss");
}
