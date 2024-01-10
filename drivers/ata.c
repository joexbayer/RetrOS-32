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
#include <arch/interrupts.h>
#include <memory.h>
#include <arch/io.h>
#include <diskdev.h>
#include <libc.h>

#include <kutils.h>

#include <serial.h>

static uint8_t* ata_driver_data;
struct ide_device ata_ide_device;

void __int_handler ata_primary()
{
}

void __int_handler ata_secondary()
{
}

static void __ide_set_drive(uint8_t bus, uint8_t i)
{
	if(bus == ATA_PRIMARY)
		if(i == ATA_MASTER)
			outportb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);

    /* ELSE UNSUPPORTED. */
}

static int ata_status_wait(int io_base, int timeout) {
	int status;

	if (timeout > 0) {
		int i = 0;
		while ((status = inportb(io_base + ATA_REG_STATUS)) & ATA_SR_BSY && (i < timeout)) i++;
	} else {
		while ((status = inportb(io_base + ATA_REG_STATUS)) & ATA_SR_BSY);
	}
	return status;
}

static void ata_io_wait(int io_base) {
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
}

static int ata_wait(int io, int adv)
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

static int __ata_read_sector(char *buf, int lba)
{
    uint16_t io = ATA_PRIMARY_IO;

    uint8_t cmd = 0xE0;
    int errors = 0;

__ata_read_sector_try_again:
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

        goto __ata_read_sector_try_again;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t d = inportw(io + ATA_REG_DATA);
        *(uint16_t *)(buf + i * 2) = d;
    }

    ata_wait(io, 0);
    return 0;
}

static int __ata_write_sector(uint16_t *buf, int lba)
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

int ata_write(char *buf, uint32_t from, uint32_t count)
{
    unsigned long pos = from;

    ENTER_CRITICAL();

    for (uint32_t i = 0; i < count; i++)
    {
        __ata_write_sector((uint16_t*) buf, pos + i);
        buf += 512;
        for (int j = 0; j < 1000; j ++)
            ;
    }
    
    LEAVE_CRITICAL();
    return count;
}

int ata_read(char *buf, uint32_t from, uint32_t numsects)
{
	unsigned long pos = from;
    int rc = 0;

    ENTER_CRITICAL();

    for (uint32_t i = 0; i < numsects; i++)
    {
        rc = __ata_read_sector((char*) buf, pos + i);
        if (rc == -1){
            LEAVE_CRITICAL();
            return -1;
        }
        buf += 512;
    }

    LEAVE_CRITICAL();
    return rc;
}

void ata_ide_init(struct pci_device* dev)
{
	interrupt_install_handler(ATA_PRIMARY_IRQ, &ata_primary);
	interrupt_install_handler(ATA_SECONDARY_IRQ, &ata_secondary);

	ata_driver_data = kalloc(512);
    if(ata_driver_data == NULL){
        dbgprintf("[ATA]: Failed to allocate memory for driver data.\n");
        return;
    }

	uint16_t io;
	__ide_set_drive(ATA_PRIMARY, ATA_MASTER);
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
			dbgprintf("[ATA]: Error while reading status\n");
			return;
		}
		while(!(status & ATA_SR_DRQ)) goto ata_status;

		dbgprintf("[ATA]: Driver ready.\n");

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
        

        attach_disk_dev(&ata_read, &ata_write, &ata_ide_device);
	}

}