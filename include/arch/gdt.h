#ifndef ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9
#define ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9

#include <stdint.h>


#define KERNEL_PRIVILEGE            0
#define PROCESSS_PRIVILEGE          3
#define GDT_KERNEL_CODE             1
#define GDT_KERNEL_DATA             2
#define GDT_PROCESS_CODE            3
#define GDT_PROCESS_DATA            4
#define GDT_TSS_INDEX               5

#define GDT_KERNEL_CS               (GDT_KERNEL_CODE << 3)
#define GDT_KERNEL_DS               (GDT_KERNEL_DATA << 3)
#define GDT_PROCESS_CS              (GDT_PROCESS_CODE << 3)
#define GDT_PROCESS_DS              (GDT_PROCESS_DATA << 3)
#define GDT_KERNEL_TSS              (GDT_TSS_INDEX << 3)

#define GDT_CODE_SEGMENT            0x0A
#define GDT_DATA_SEGMENT            0x02
#define GDT_TSS_SEGMENT             0x09

#define MEMORY                      1
#define SYSTEM                      0

#define TSS_SIZE                    103


void init_gdt();

struct gdt_segment {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_hi;
    uint8_t base_hi;
} __attribute__ ((packed));

#endif /* ABB55FA7_D0EB_44AB_9054_17CC84EDC4B9 */
