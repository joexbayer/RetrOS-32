#ifndef __ATAPI_H__
#define __ATAPI_H__

#include <stdint.h>
#include <pci.h>
#include <ata.h>

// ATA/ATAPI Registers
#define ATA_PRIMARY_IO       0x1F0
#define ATA_SECONDARY_IO     0x170

#define ATA_REG_ERROR        0x1
#define ATA_REG_SECCOUNT     0x2
#define ATA_REG_LBA_LOW      0x3
#define ATA_REG_LBA_MID      0x4
#define ATA_REG_LBA_HIGH     0x5
#define ATA_REG_DRIVESEL     0x6
#define ATA_REG_COMMAND      0x7  // Write-only, same as STATUS

// ATA/ATAPI Commands
#define ATA_CMD_DEVICE_RESET 0x08
#define ATAPI_IDENTIFY       0xA1
#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_READ_CAPACITY 0x25
#define ATAPI_CMD_PACKET 0xA0

#define ATA_REG_DEVCTRL 0x206

// ATA/ATAPI Status Register Flags
#define ATA_STATUS_ERR       0x01  // Error
#define ATA_STATUS_DRQ       0x08  // Data Request (ready for data transfer)
#define ATA_STATUS_SRV       0x10  // Overlapped Mode Service Request
#define ATA_STATUS_DF        0x20  // Drive Fault Error (does not set ERR)
#define ATA_STATUS_RDY       0x40  // Drive is ready
#define ATA_STATUS_BSY       0x80  // Drive is busy

// ATAPI Error Register Flags (after sending IDENTIFY PACKET DEVICE)
#define ATAPI_ERR_ABRT       0x04  // Command aborted

#define ATAPI_SECTOR_SIZE 2048
#define VIRTUAL_SECTOR_SIZE 512
#define CACHE_SIZE 4  /* Number of 2048-byte sectors to cache */


int atapi_init(struct pci_device* dev);

#endif // !__ATAPI_H__