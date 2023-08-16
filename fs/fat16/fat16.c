/**
 * @file fat16.c
 * @author Joe Bayer (joexbayer)
 * @brief Main API for the FAT16 Filesystem.
 * @version 0.1
 * @date 2023-05-31
 * @see http://www.tavi.co.uk/phobos/fat.html
 * @see https://wiki.osdev.org/FAT
 * @copyright Copyright (c) 2023
 * 
 */
#include <fs/fat16.h>
#include <kutils.h>
#include <diskdev.h>

static struct fat_boot_table boot_table;

static uint16_t fat16_get_fat_entry(uint32_t cluster)
{
    byte_t buffer[512];
    uint32_t fat_offset = cluster * 2;  // Each entry is 2 bytes
    uint32_t fat_sector = 1 + (fat_offset / 512);  // Add 1 to skip boot sector
    uint32_t entry_offset = fat_offset % 512;

    //device.read_block(fat_sector, buffer);
    return *(uint16_t*)&buffer[entry_offset];
}

static void fat16_set_fat_entry(uint32_t cluster, uint16_t value)
{
    byte_t buffer[512];
    uint32_t fat_offset = cluster * 2;  // Each entry is 2 bytes
    uint32_t fat_sector = 1 + (fat_offset / 512);  // Add 1 to skip boot sector
    uint32_t entry_offset = fat_offset % 512;

    //device.read_block(fat_sector, buffer);
    *(uint16_t*)&buffer[entry_offset] = value;
    //device.write_block(fat_sector, buffer);
}

static void fat16_allocate_cluster(uint32_t cluster)
{
    store_fat_entry(cluster, 0xFFFF);  // marking cluster as end of file (for now)
}

static void fat16_free_cluster(uint32_t cluster)
{
    store_fat_entry(cluster, 0x0000);  // marking cluster as free
}

static uint32_t fat16_get_first_free_cluster()
{
    for (uint32_t i = 2; i < 65536; i++) {  // Start from 2 as 0 and 1 are reserved entries
        if (fetch_fat_entry(i) == 0x0000) {
            return i;
        }
    }
    return 0;  // no free cluster found
}

int fat16_format()
{
    int total_blocks = disk_size()/512;

    struct fat_boot_table new_boot_table = {
        .manufacturer = "NETOS  ",  // This can be any 8-character string
        .bytes_per_plock = 512,      // Standard block size
        .blocks_per_allocation = 1,  // Usually 1 for small devices
        .reserved_blocks = 1,        // The boot block
        .num_FATs = 2,               // Standard for FAT16
        .root_dir_entries = 512,     // This gives you 512 * 32B = 16KB root directory space
        .total_blocks = total_blocks,
        .media_descriptor = 0xF8,    // Fixed disk (not removable)
        .fat_blocks = (total_blocks - (1 + boot_table.root_dir_entries * 32 / 512)) / 2,
        // ... other fields ...
        .file_system_identifier = "FAT16   "
    };

    // Write the boot table to the boot block
    //dev->write_block(BOOT_BLOCK, (uint8_t *)&new_boot_table);

    // Clear out the FAT tables
    uint8_t zero_block[512];
    memset(zero_block, 0, sizeof(zero_block));
    for (uint16_t i = 0; i < new_boot_table.fat_blocks; i++) {
        //dev->write_block(FAT1_START_BLOCK + i, zero_block);
        //dev->write_block(FAT2_START_BLOCK + i, zero_block);
    }

    // Clear out the root directory area
    for (uint16_t i = 0; i < new_boot_table.root_dir_entries * 32 / 512; i++) {
        //dev->write_block(ROOT_DIRECTORY_START_BLOCK + i, zero_block);
    }

    // Update the boot table
    boot_table = new_boot_table;

    return 0;  // assume success
}



void fat16_set_time(uint16_t *time, ubyte_t hours, ubyte_t minutes, ubyte_t seconds)
{
    /* Clear the existing time bits */
    *time &= 0xFC00;
    /* Set the hours bits */
    *time |= (hours & 0x1F) << 11;
    /* Set the minutes bits */
    *time |= (minutes & 0x3F) << 5;

    /* Set the seconds bits (converted to two-second periods) */
    ubyte_t twoSecondPeriods = seconds / 2;
    *time |= twoSecondPeriods & 0x1F;
}

void fat16_set_date(uint16_t *date, uint16_t year, ubyte_t month, ubyte_t day)
{
    /* Clear the existing date bits */
    *date &= 0xFE00;
    /* Set the year bits (offset from 1980) */
    *date |= ((year - 1980) & 0x7F) << 9;
    /* Set the month bits */
    *date |= (month & 0x0F) << 5;
    /* Set the day bits */
    *date |= (day & 0x1F);
}

int fat16_initialize()
{
    // load the bootblock

    return -1;
}

/* Open a file. Returns a file descriptor or a negative value on error. */
int fat16_open(const char *path);

/* Read from an open file. Returns the number of bytes read. */
int fat16_read(int fd, void *buffer, size_t count);

/* Write to an open file. Returns the number of bytes written. */
int fat16_write(int fd, const void *buffer, size_t count);

/* Close an open file. Returns 0 on success, and a negative value on error. */
int fat16_close(int fd);

/* Create a new directory. Returns 0 on success, and a negative value on error. */
int fat16_mkdir(const char *path);

/* Delete a file or directory. Returns 0 on success, and a negative value on error. */
int fat16_remove(const char *path);

/* List the contents of a directory. */
int fat16_listdir(const char *path, void (*callback)(const char *name, int is_directory));


/* Blocks before ROOT directory: (size of FAT)*(number of FATs) + 1 */