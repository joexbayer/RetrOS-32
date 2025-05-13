#include <atapi.h>
#include <stdint.h>
#include <kutils.h>
#include <arch/io.h>
#include <memory.h>
#include <pci.h>
#include <diskdev.h>
#include <serial.h>

static struct ide_device atapi_ide_device;

struct atapi_cache_entry {
    uint32_t lba;  /* Logical Block Address of the cached sector */
    uint8_t data[ATAPI_SECTOR_SIZE];
    bool_t valid;
} cache[CACHE_SIZE];

void init_cache()
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = 0;
    }
}

int find_in_cache(uint32_t lba)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].lba == lba) {
            return i;
        }
    }
    return -1;  /* Not found */
}

int find_empty_cache_slot()
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (!cache[i].valid) {
            return i;
        }
    }
    return -1;  /* No empty slot */
}

/* DRIVER CODE  */

static void ata_io_wait(int io_base) {
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
	inportb(io_base + ATA_REG_ALTSTATUS);
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

static void ata_wait_busy(uint16_t base)
{
    ata_wait(base, 0);
    while(inportb(base + ATA_REG_STATUS) & 0x80); /* Wait while busy */
}

static void ata_wait_drq(uint16_t base)
{
    ata_wait(base, 0);
    while(!(inportb(base + ATA_REG_STATUS) & 0x08)); /* Wait until DRQ is set */
}

static void read_atapi_sector(uint8_t *buffer, uint32_t lba)
{
    uint16_t base = ATA_PRIMARY_IO;
    ata_wait_busy(base);

    /* Set up registers for ATAPI read */
    outportb(base + ATA_REG_FEATURES, 0x0); /* No overlap or DMA */
    outportb(base + ATA_REG_SECCOUNT, 1); /* Read one sector */
    outportb(base + ATA_REG_LBA_LOW, lba & 0xFF);
    outportb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outportb(base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outportb(base + ATA_REG_DRIVESEL, 0xB0); /* Assuming master drive */
    outportb(base + ATA_REG_COMMAND, ATAPI_CMD_READ);

    ata_wait_drq(base);

    for (int i = 0; i < ATAPI_SECTOR_SIZE/2; i++) {
        uint16_t data = inportw(base + ATA_REG_DATA);
        buffer[i*2] = data & 0xFF;
        buffer[i*2 + 1] = (data >> 8) & 0xFF;
    }
}

static void read_virtual_sector(uint8_t *buffer, uint32_t lba)
{
    uint32_t atapi_lba = lba / 4;
    uint32_t offset = (lba % 4) * VIRTUAL_SECTOR_SIZE;

    int cache_index = find_in_cache(atapi_lba);
    if (cache_index == -1) {
        cache_index = find_empty_cache_slot();
        if (cache_index == -1) {
            cache_index = 0; /* Simple cache replacement strategy: overwrite the first entry */
        }

        read_atapi_sector((uint8_t*)atapi_lba, (uint32_t)cache[cache_index].data);
        cache[cache_index].lba = atapi_lba;
        cache[cache_index].valid = true;
    }

    memcpy(buffer, &cache[cache_index].data[offset], VIRTUAL_SECTOR_SIZE);
}

static uint32_t atapi_read_capacity(uint16_t base) {
    uint8_t packet[12] = {0};
    packet[0] = ATAPI_CMD_READ_CAPACITY;

    /* Send the packet command */
    for (int i = 0; i < 6; i++) {
        outportw(base + ATA_REG_DATA, *(uint16_t*)(packet + i * 2));
    }

    /* Wait for the device to process the command and set DRQ */
    ata_wait_drq(base);

    /* Read the response */
    uint32_t total_sectors = inportl(base + ATA_REG_DATA);
    uint32_t sector_size = inportl(base + ATA_REG_DATA);
    (void)sector_size;  /* Ignore sector size for now */

    /* For CD-ROMs, the sector size is typically 2048 bytes. */
    /* The total capacity is total_sectors * sector_size. */

    return total_sectors;  /* or return total_sectors * sector_size for total capacity in bytes */
}

static void __ide_set_drive(uint8_t bus, uint8_t i)
{
	if(bus == ATA_PRIMARY)
		if(i == ATA_MASTER)
			outportb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);

    /* ELSE UNSUPPORTED. */
}

/* Assuming the ATAPI device is on the primary channel, master drive */
int atapi_init(struct pci_device* dev)
{
    init_cache();

    uint8_t status;
    uint16_t base = ATA_PRIMARY_IO;

    outportb(base + ATA_REG_DEVCTRL, 0x04);  /* Assert the SRST bit to reset the device */
    ata_io_wait(base);  /* Wait for a short duration */
    outportb(base + ATA_REG_DEVCTRL, 0x00);  /* Clear the SRST bit */

    /* Check for device presence */
    uint8_t initial_status = inportb(base + ATA_REG_STATUS);
    if (initial_status == 0xFF) {
        dbgprintf("[ATAPI] No device detected on this port.\n");
        return -1;
    }
    dbgprintf("[ATAPI] Device detected on this port.\n");

    /* Select the ATAPI device */
    outportb(base + ATA_REG_DRIVESEL, 0xA0); /* Select master drive */

    /* Send IDENTIFY PACKET DEVICE command */
    outportb(base + ATA_REG_COMMAND, ATAPI_IDENTIFY);

    dbgprintf("[ATAPI] Waiting for device to be ready...\n");

    /* Wait for the device to be ready */
    ata_wait_busy(base);

    /* Check the status register */
    status = inportb(base + ATA_REG_STATUS);
    if (!(status & 0x08)) {
        dbgprintf("[ATAPI] DRQ bit not set. Status: 0x%x\n", status);
        if (status & 0x01) {
            uint8_t error = inportb(base + ATA_REG_ERROR);
            dbgprintf("[ATAPI] ERR bit set. Error register: 0x%x\n", error);
            return -error;
        }
        return -1;
    }
    dbgprintf("[ATAPI] Device status: 0x%x\n", status);

    /* Read identification data */
    uint16_t id_data[256];
    for (int i = 0; i < 256; i++) {
        id_data[i] = inportw(base + ATA_REG_DATA);
    }

    /* Extract the model string */
    for (int i = 0; i < 20; i++) {
        atapi_ide_device.model[i * 2] = id_data[27 + i] >> 8;
        atapi_ide_device.model[i * 2 + 1] = id_data[27 + i] & 0xFF;
    }
    atapi_ide_device.model[40] = '\0';  /* Null-terminate the string */

    dbgprintf("[ATAPI] Device model: %s\n", atapi_ide_device.model);


    (void)atapi_read_capacity(base);  /* Read capacity to check if the device is ready */
    //attach_disk_dev(&read_virtual_sector, NULL, &atapi_ide_device);

    (void)__ide_set_drive(ATA_PRIMARY, ATA_MASTER);
    (void)read_virtual_sector(NULL, 0);  /* Test read_virtual_sector function */

    return 0;
}
