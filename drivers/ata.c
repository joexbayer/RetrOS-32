/**
 * @file ata.c
 * @author Joe Bayer (joexbayer)
 * @brief ATA driver for non volatile storage. Using PIO for simplicity.
 * @see http://learnitonweb.com/2020/05/22/12-developing-an-operating-system-tutorial-episode-6-ata-pio-driver-osdev/
 * @version 0.1
 * @date 2022-06-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <ata.h>
#include <util.h>

#define STATUS_BSY 0x80 /* Busy executing command. */
#define STATUS_RDY 0x40 /* Ready. */
#define STATUS_DRQ 0x08 /* Expecting Data. */
#define STATUS_DF 0x20 
#define STATUS_ERR 0x01 /* Error. */

static void __ATA_wait_BSY()   //Wait for bsy to be 0
{
	while(inportb(0x1F7) & STATUS_BSY);
}
static void __ATA_wait_DRQ()  //Wait fot drq to be 1
{
	while(!(inportb(0x1F7) & STATUS_RDY));
}

/**
 * @brief Reads a sector into given address, based on offset.
 * 
 * @param target_address address to write data into
 * @param offset sector offset
 * @param sector_count sectors to read.
 */
void read_sectors_ATA_PIO(void* target_address, uint32_t offset, uint8_t sector_count)
{

	__ATA_wait_BSY();
	outportb(0x1F6,0xE0 | ((offset >>24) & 0xF));
	outportb(0x1F2, sector_count);
	outportb(0x1F3, (uint8_t) offset);
	outportb(0x1F4, (uint8_t)(offset >> 8));
	outportb(0x1F5, (uint8_t)(offset >> 16)); 
	outportb(0x1F7,0x20); //Send the read command

	uint16_t *target = (uint16_t*) target_address;

	for (int j = 0; j < sector_count; j++)
	{
		__ATA_wait_BSY();
		__ATA_wait_DRQ();
		for(int i = 0; i < 512; i++)
			target[i] = inportw(0x1F0);
		target += 512;
	}
}


void write_sectors_ATA_PIO(uint32_t* buffer, uint32_t offset, uint8_t sector_count)
{
	__ATA_wait_BSY();
	outportb(0x1F6,0xE0 | ((offset >>24) & 0xF));
	outportb(0x1F2,sector_count);
	outportb(0x1F3, (uint8_t) offset);
	outportb(0x1F4, (uint8_t)(offset >> 8));
	outportb(0x1F5, (uint8_t)(offset >> 16)); 
	outportb(0x1F7,0x30); //Send the write command

	for (int j = 0; j < sector_count; j++)
	{
		__ATA_wait_BSY();
		__ATA_wait_DRQ();
		for(int i = 0; i < 512; i++)
		{
			outportl(0x1F0, buffer[i]);
		}
	}
}