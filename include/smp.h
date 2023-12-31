#ifndef __SMP_H
#define __SMP_H

/**
 * @file smp.h
 * @author Joe Bayer (joexbayer)
 * @brief Setup for SMP (Symmetric Multiprocessing)
 * Structs from osdev.org
 * @version 0.1
 * @date 2023-02-10
 * @see https://wiki.osdev.org/Symmetric_Multiprocessing
 * @see http://datasheets.chipdb.org/Intel/x86/Intel%20Architecture/24201606.PDF
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdint.h>

struct mp_info {
    char signature[4];
    uint32_t table;
    uint8_t length;
    uint8_t specification;
    uint8_t checksum;
    uint8_t configuration;
    uint32_t features;
};

struct mp_table {
    char signature[4]; /* "PCMP" */
    uint16_t length;
    uint8_t specification;
    uint8_t checksum;
    char oem_id[8];
    char product_id[12];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t lapic_address;
    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
};

struct entry_processor {
    uint8_t type; /* Always 0 */
    uint8_t local_apic_id;
    uint8_t local_apic_version;
    uint8_t flags; /* If bit 0 is clear then the processor must be ignored */
                   /* If bit 1 is set then the processor is the bootstrap processor */
    uint32_t signature;
    uint32_t feature_flags;
    uint64_t reserved;
};

struct entry_io_apic {
    uint8_t type; /* Always 2 */
    uint8_t id;
    uint8_t version;
    uint8_t flags; /* If bit 0 is set then the entry should be ignored */
    uint32_t address; /* The memory mapped address of the IO APIC is memory */
};

struct entry_bus {
    uint8_t type; /* Always 1 */
    uint8_t bus_id;
    char bus_type[6]; /* Identifier string (e.g., "ISA   ", "PCI   ", etc.) */
};

struct entry_io_interrupt {
    uint8_t type; /* Always 3 */
    uint8_t interrupt_type;
    uint16_t flags;
    uint8_t source_bus_id;
    uint8_t source_bus_irq;
    uint8_t destination_io_apic_id;
    uint8_t destination_io_apic_intin;
};

struct entry_local_interrupt {
    uint8_t type; /* Always 4 */
    uint8_t interrupt_type;
    uint16_t flags;
    uint8_t source_bus_id;
    uint8_t source_bus_irq;
    uint8_t destination_local_apic_id;
    uint8_t destination_local_apic_lintin;
};


int smp_parse();
struct mp_info* find_mp_floating_ptr();

#endif /* !__SMP_H */
