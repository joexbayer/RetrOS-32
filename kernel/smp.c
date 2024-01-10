/**
 * @file smp.c
 * @author Joe Bayer (joexbayer)
 * @brief Symmetric multiprocessing.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <smp.h>
#include <libc.h>
#include <kutils.h>
#include <serial.h>

static void smp_print_processor(const struct entry_processor* proc)
{
    dbgprintf("Processor: APIC ID=%d, Flags=0x%x, Signature=0x%x, Features=0x%x\n",
           proc->local_apic_id, proc->flags, proc->signature, proc->feature_flags);
}

static void smp_print_io_apic_info(const struct entry_io_apic* io_apic) {
    dbgprintf("I/O APIC: ID=%d, Address=0x%x\n", io_apic->id, io_apic->address);
}

static void smp_print_bus_info(const struct entry_bus* bus) {
    char bus_type_str[7];
    memcpy(bus_type_str, bus->bus_type, 6);
    bus_type_str[6] = '\0'; /* Ensure null-termination */
    dbgprintf("Bus: ID=%d, Type=%s\n", bus->bus_id, bus_type_str);
}

static void smp_print_io_interrupt_info(const struct entry_io_interrupt* io_int) {
    //dbgprintf("I/O Interrupt: Type=%d, Flags=0x%x, Source Bus ID=%d, Source IRQ=%d, Dest IO APIC ID=%d, Dest IO APIC INTIN=%d\n",
    //          io_int->interrupt_type, io_int->flags, io_int->source_bus_id, io_int->source_bus_irq, io_int->destination_io_apic_id, io_int->destination_io_apic_intin);
}

static void smp_print_local_interrupt_info(const struct entry_local_interrupt* local_int) {
    //dbgprintf("Local Interrupt: Type=%d, Flags=0x%x, Source Bus ID=%d, Source IRQ=%d, Dest Local APIC ID=%d, Dest Local APIC LINTIN=%d\n",
    //          local_int->interrupt_type, local_int->flags, local_int->source_bus_id, local_int->source_bus_irq, local_int->destination_local_apic_id, local_int->destination_local_apic_lintin);
}


struct mp_info* find_mp_floating_ptr()
{
    /* Typically, the MP Floating Pointer Structure is located in one of the following: */
    /* 1. The first kilobyte of the Extended BIOS Data Area (EBDA) */
    /* 2. The last kilobyte of system base memory */
    /* 3. The BIOS ROM between 0xE0000 and 0xFFFFF */

    /* For simplicity, this example will only search in the BIOS ROM area. */
    for (uintptr_t addr = 0xE0000; addr < 0xFFFFF; addr += 16) {
        struct mp_info* mp_ptr = ( struct mp_info* )addr;
        if (memcmp(mp_ptr->signature, "_MP_", 4) == 0) {
            /* Validate checksum here... */
            return mp_ptr;
        }
    }
    return NULL;
}

int smp_parse()
{
    struct mp_info* mp_ptr = find_mp_floating_ptr();
    if (mp_ptr == NULL) {
        return -1;
    }

    dbgprintf("MP Floating Pointer Structure found at 0x%x\n", mp_ptr);

    /* The MP Floating Pointer Structure contains the physical address of the MP Configuration Table. */
    struct mp_table* mp_table = ( struct mp_table* )mp_ptr->table;
    if (memcmp(mp_table->signature, "PCMP", 4) != 0) {
        return -1;
    }

    /* Start parsing the MP Configuration Table */
    uint8_t* entries = (uint8_t*)(mp_table + 1); /* Pointer to the first entry */
    for (int i = 0; i < mp_table->entry_count; i++) {
        switch (entries[0]) {  /* First byte is the entry type */
        case 0:  /* Processor Entry */
            smp_print_processor((struct entry_processor*)entries);
            entries += sizeof(struct entry_processor);
            break;
        case 1:  /* Bus Entry */
            smp_print_bus_info((struct entry_bus*)entries);
            entries += sizeof(struct entry_bus);
            break;
        case 2:  /* I/O APIC Entry */
            smp_print_io_apic_info((struct entry_io_apic*)entries);
            entries += sizeof(struct entry_io_apic);
            break;
        case 3:  /* I/O Interrupt Assignment Entry */
            smp_print_io_interrupt_info((struct entry_io_interrupt*)entries);
            entries += sizeof(struct entry_io_interrupt);
            break;
        case 4:  /* Local Interrupt Assignment Entry */
            smp_print_local_interrupt_info((struct entry_local_interrupt*)entries);
            entries += sizeof(struct entry_local_interrupt);
            break;
        default:
            dbgprintf("Unknown entry type: %d\n", entries[0]);
            /* Handle unknown entry type or skip it */
            /* You might want to increment 'entries' by a default size or based on specific criteria */
            break;
        }

    }

    return 0;  /* Successful initialization */
}
