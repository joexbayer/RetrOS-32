#ifndef ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9
#define ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9

#include <stdint.h>

void init_gdt();
void gdt_set_gate(uint32_t,uint32_t,uint32_t, uint8_t ,uint8_t);

struct gdt_entry
{
   uint16_t limit_low;           /*  The lower 16 bits of the limit. */
   uint16_t base_low;            /*  The lower 16 bits of the base. */
   uint8_t  base_middle;         /*  The next 8 bits of the base. */
   uint8_t  access;              /*  Access flags, determine what ring this segment can be used in. */
   uint8_t  granularity;
   uint8_t  base_high;           /*  The last 8 bits of the base. */
} __attribute__((packed));

#endif /* ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9 */
