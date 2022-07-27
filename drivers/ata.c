/**
 * @file ata.c
 * @author Joe Bayer (joexbayer)
 * @brief ATA driver for non volatile storage. Using PIO for simplicity.
 * @see https://wiki.osdev.org/PCI_IDE_Controller
 * @version 0.1
 * @date 2022-06-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <ata.h>
#include <terminal.h>
#include <interrupts.h>
#include <memory.h>
#include <util.h>

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

static uint8_t* ata_driver_data;
static struct ide_device ata_ide_device;

void ata_primary()
{
}

void ata_secondary()
{
}

void ide_select_drive(uint8_t bus, uint8_t i)
{
	if(bus == ATA_PRIMARY)
		if(i == ATA_MASTER)
			outportb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);
		else outportb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xB0);
	else
		if(i == ATA_MASTER)
			outportb(ATA_SECONDARY_IO + ATA_REG_HDDEVSEL, 0xA0);
		else outportb(ATA_SECONDARY_IO + ATA_REG_HDDEVSEL, 0xB0);
}

int ata_status_wait(int io_base, int timeout) {
	int status;

	if (timeout > 0) {
		int i = 0;
		while ((status = inportb(io_base + ATA_REG_STATUS)) & ATA_SR_BSY && (i < timeout)) i++;
	} else {
		while ((status = inportb(io_base + ATA_REG_STATUS)) & ATA_SR_BSY);
	}
	return status;
}

void ata_io_wait(int io_base) {
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
}

int ata_wait(int io, int adv)
{
    uint8_t status = 0;

    ata_io_wait(io);

    status = ata_status_wait(io, -1);

    if (adv) {
        status = inportb(io + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return 1;
        if (status & ATA_SR_DF)  return 1;
        if (!(status & ATA_SR_DRQ)) return 1;
    }

    return 0;
}

int __ata_read_sector(char *buf, int lba)
{
    uint16_t io = ATA_PRIMARY_IO;

    uint8_t cmd = 0xE0;
    int errors = 0;

try_a:
    outportb(io + ATA_REG_CONTROL, 0x02);

    ata_wait(io, 0);

    outportb(io + ATA_REG_HDDEVSEL, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
    outportb(io + ATA_REG_FEATURES, 0x00);
    outportb(io + ATA_REG_SECCOUNT0, 1);
    outportb(io + ATA_REG_LBA0, (uint8_t)(lba));
    outportb(io + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outportb(io + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outportb(io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    if (ata_wait(io, 1)) {
        errors ++;
        if (errors > 4)
            return -1;

        goto try_a;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t d = inportw(io + ATA_REG_DATA);
        *(uint16_t *)(buf + i * 2) = d;
    }

    ata_wait(io, 0);
    return 0;
}

int __ata_write_sector(uint16_t *buf, int lba)
{
    uint16_t io = ATA_PRIMARY_IO;

    uint8_t cmd = 0xE0;

    outportb(io + ATA_REG_CONTROL, 0x02);

    ata_wait(io, 0);

    outportb(io + ATA_REG_HDDEVSEL, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
    ata_wait(io, 0);
    outportb(io + ATA_REG_FEATURES, 0x00);
    outportb(io + ATA_REG_SECCOUNT0, 1);
    outportb(io + ATA_REG_LBA0, (uint8_t)(lba));
    outportb(io + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outportb(io + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outportb(io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ata_wait(io, 0);

    for (int i = 0; i < 256; i++) {
        outportw(io + ATA_REG_DATA, buf[i]);
        asm volatile("nop; nop; nop");
    }
    outportb(io + 0x07, ATA_CMD_CACHE_FLUSH);

    ata_wait(io, 0);

    return 0;
}

int ata_write(uint8_t *buf, int count)
{
    unsigned long pos = 0;

    CLI();

    for (int i = 0; i < count; i++)
    {
        __ata_write_sector((uint16_t*) buf, pos + i);
        buf += 512;
        for (int j = 0; j < 1000; j ++)
            ;
    }
    
    STI();
    return count;
}

int ata_read(uint8_t *buf, int numsects)
{
	unsigned long pos = 0;
    int rc = 0, read = 0;

    CLI();

    for (int i = 0; i < numsects; i++)
    {
        rc = __ata_read_sector((uint16_t*) buf, pos + i);
        if (rc == -1)
            return -1;
        buf += 512;
        read += 512;
    }

    STI();
    return rc;
}


void ata_ide_init()
{
	isr_install(ATA_PRIMARY_IRQ, &ata_primary);
	isr_install(ATA_SECONDARY_IRQ, &ata_secondary);

	ata_driver_data = alloc(512);

	uint16_t io;
	ide_select_drive(ATA_PRIMARY, ATA_MASTER);
	io = ATA_PRIMARY_IO;

	outportb(io + ATA_REG_SECCOUNT0, 0);
	outportb(io + ATA_REG_LBA0, 0);
	outportb(io + ATA_REG_LBA1, 0);
	outportb(io + ATA_REG_LBA2, 0);

	/* Send IDENTIFY to ATA */
	outportb(io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

	/* Read status port till BSY is clear*/
	uint8_t status = inportb(io + ATA_REG_STATUS);
	if(status){
		while((inportb(io + ATA_REG_STATUS) & ATA_SR_BSY) != 0);
		ata_status: status = inportb(io + ATA_REG_STATUS);

		if(status & ATA_SR_ERR)
		{
			twriteln("ATA: Error while reading status");
			return;
		}
		while(!(status & ATA_SR_DRQ)) goto ata_status;

		twriteln("ATA: Driver ready.");

		for(int i = 0; i < 256; i++)
		{
			*(uint16_t *)(ata_driver_data + i*2) = inportw(io + ATA_REG_DATA);
		}

		for(int i = 0; i < ATA_IDE_MODEL_LENGTH-1; i += 2)
		{
			ata_ide_device.model[i] = ata_driver_data[ATA_IDENT_MODEL + i + 1];
			ata_ide_device.model[i + 1] = ata_driver_data[ATA_IDENT_MODEL + i];
		}

		ata_ide_device.drive = (ATA_PRIMARY << 1) | ATA_MASTER;
		ata_ide_device.signature = *((unsigned short *)(ata_driver_data + ATA_IDENT_DEVICETYPE));
		ata_ide_device.commandSets = *((unsigned int *)(ata_driver_data + ATA_IDENT_COMMANDSETS));

		if (ata_ide_device.commandSets & (1 << 26))
            // Device uses 48-Bit Addressing:
            ata_ide_device.size   = *((unsigned int *)(ata_driver_data + ATA_IDENT_MAX_LBA_EXT));
         else
            // Device uses CHS or 28-bit Addressing:
            ata_ide_device.size   = *((unsigned int *)(ata_driver_data + ATA_IDENT_MAX_LBA));

		twritef("ATA: %s\n", ata_ide_device.model);
		twritef("ATA: %d\n", ata_ide_device.size*512);

	}

}