#ifndef ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9
#define ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9

#include <stdint.h>

void init_gdt();

extern struct tss_entry tss;

struct gdt_segment {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high;
    uint8_t base_high;
} __attribute__ ((packed));



enum {
   KERNEL_CODE = 1,
   KERNEL_DATA,
   PROCESS_CODE,
   PROCESS_DATA,
   TSS_INDEX,

   KERNEL_CS = KERNEL_CODE << 3,
   KERNEL_DS = KERNEL_DATA << 3,
   PROCESS_CS = PROCESS_CODE << 3,
   PROCESS_DS = PROCESS_DATA << 3,
   KERNEL_TSS = TSS_INDEX << 3,

   /* descriptor types */
   CODE_SEGMENT = 0x0A,
   DATA_SEGMENT = 0x02,
   TSS_SEGMENT = 0x09,

   MEMORY = 1,
   SYSTEM = 0,

   TSS_SIZE = 103
};
#endif /* ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9 */
