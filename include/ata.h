#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <pci.h>

#define ATA_PRIMARY_IRQ 14
#define ATA_SECONDARY_IRQ 15

#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170

#define ATA_PRIMARY_DCR_AS 0x3F6
#define ATA_SECONDARY_DCR_AS 0x376

#define ATA_PRIMARY      0x00
#define ATA_SECONDARY    0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_STATUS     0x07
#define ATA_REG_ALTSTATUS  0x0C

#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05

#define ATA_REG_COMMAND    0x07
#define ATA_REG_DATA       0x00
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_FEATURES   0x01

#define ATA_CMD_IDENTIFY          0xEC
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_WRITE_PIO         0x30

#define ATA_SR_BSY     0x80	   // BUSY
#define ATA_SR_ERR     0x01    // Error
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_DF      0x20    // Drive write fault


#define ATA_IDENT_DEVICETYPE   0

#define ATA_IDENT_MODEL        54
#define ATA_IDE_MODEL_LENGTH 41
#define ATA_IDENT_COMMANDSETS  164

#define ATA_IDENT_MAX_LBA_EXT  200
#define ATA_IDENT_MAX_LBA      120

struct ide_device {
   unsigned char  reserved;    // 0 (Empty) or 1 (This Drive really exists).
   unsigned char  channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
   unsigned char  drive;       // 0 (Master Drive) or 1 (Slave Drive).
   unsigned short type;        // 0: ATA, 1:ATAPI.
   unsigned short signature;   // Drive Signature
   unsigned short capabilities;// Features.
   unsigned int   commandSets; // Command Sets Supported.
   unsigned int   size;        // Size in Sectors.
   unsigned char  model[ATA_IDE_MODEL_LENGTH];   // Model in string.
};

void ata_ide_init(struct pci_device* dev);

#endif /* ATA_H */

