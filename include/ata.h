#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void ata_ide_init();
int ata_read(uint8_t *buf, int numsects);
int ata_write(uint8_t *buf, int count);

#endif /* ATA_H */
