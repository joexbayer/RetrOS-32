#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void read_sectors_ATA_PIO(void* target_address, uint32_t offset, uint8_t sector_count);
void write_sectors_ATA_PIO(uint32_t* buffer, uint32_t offset, uint8_t sector_count);

#endif /* ATA_H */
