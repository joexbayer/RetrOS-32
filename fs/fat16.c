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
#include <stdint.h>
#include <fs/fat16.h>
#include <kutils.h>
#include <diskdev.h>
#include <memory.h>

#define BOOT_BLOCK 0
#define ENTRIES_PER_BLOCK (512 / sizeof(struct fat16_directory_entry))

static struct fat_boot_table boot_table = {0};
static byte_t* fat_table_memory = NULL;  /* pointer to the in-memory FAT table */

/* HELPER FUNCTIONS */

static inline uint16_t get_fat_start_block()
{
    return BOOT_BLOCK + 1;  /* FAT starts right after the boot block */
}

static inline uint16_t get_root_directory_start_block()
{
    return get_fat_start_block() + boot_table.fat_blocks;  /* Root directory starts after FAT */
}

/* Function to read a specific block of the root directory into a buffer */
static inline void fat16_read_root_directory_block(uint16_t block_index, struct fat16_directory_entry* buffer)
{
    /* Calculate the starting block of the root directory */
    uint16_t root_directory_start_block = get_root_directory_start_block();
    read_block((byte_t *)buffer, root_directory_start_block + block_index);
}

static uint16_t fat16_get_fat_entry(uint32_t cluster)
{
    if(fat_table_memory == NULL){
        return -1;
    }

    uint32_t fat_offset = cluster * 2;  /* Each entry is 2 bytes */
    return *(uint16_t*)(fat_table_memory + fat_offset);
}

static void fat16_set_fat_entry(uint32_t cluster, uint16_t value)
{
    if(fat_table_memory == NULL){
        return;
    }

    uint32_t fat_offset = cluster * 2;  /* Each entry is 2 bytes */
    *(uint16_t*)(fat_table_memory + fat_offset) = value;
}

static void fat16_sync_fat_table()
{
    if(fat_table_memory == NULL){
        return;
    }

    int start_block = get_fat_start_block();
    for (uint16_t i = 0; i < boot_table.fat_blocks; i++) {
        write_block(fat_table_memory + i * 512, start_block + i);
    }
}

/* wrapper functions TODO: inline replace */
static void fat16_allocate_cluster(uint32_t cluster)
{
    fat16_set_fat_entry(cluster, 0xFFFF);  /* marking cluster as end of file */
}

static void fat16_free_cluster(uint32_t cluster)
{
    fat16_set_fat_entry(cluster, 0x0000);  /* marking cluster as free */
}

static uint32_t fat16_get_free_cluster()
{
    for (int i = 2; i < 65536; i++) {  /* Start from 2 as 0 and 1 are reserved entries */
        if (fat16_get_fat_entry(i) == 0x0000) {
            return i;
        }
    }
    return 0;  /* no free cluster found */
}


int fat16_read_root_directory(struct fat16_directory_entry* buffer) {
    /* Calculate the number of blocks in the root directory */
    uint16_t root_directory_blocks = (boot_table.root_dir_entries + ENTRIES_PER_BLOCK - 1) / ENTRIES_PER_BLOCK;
    
    /* Read each block of the root directory into the buffer */
    for (uint16_t i = 0; i < root_directory_blocks; i++) {
        fat16_read_root_directory_block(i, buffer + i * ENTRIES_PER_BLOCK);
    }

    return root_directory_blocks;  /* Return number of blocks read */
}

int fat16_format()
{
    int total_blocks = disk_size()/512;

    struct fat_boot_table new_boot_table = {
        .manufacturer = "NETOS   ",  /* This can be any 8-character string */
        .bytes_per_plock = 512,      /* Standard block size */
        .blocks_per_allocation = 1,  /* Usually 1 for small devices */
        .reserved_blocks = 1,        /* The boot block */
        .num_FATs = 1,               /* Standard for FAT16 */
        .root_dir_entries = 512,     /* This gives you 512 * 32B = 16KB root directory space */
        .total_blocks = total_blocks,
        .media_descriptor = 0xF8,    /* Fixed disk  */
        .fat_blocks = (total_blocks - (1 + boot_table.root_dir_entries * 32 / 512)) / 2,
        /* ... other fields ... */
        .file_system_identifier = "FAT16   "
    };

    /* dbgprint out bootblock information: */
    dbgprintf("bootblock information:\n");
    dbgprintf("manufacturer: %s\n", new_boot_table.manufacturer);
    dbgprintf("bytes_per_plock: %d\n", new_boot_table.bytes_per_plock);
    dbgprintf("blocks_per_allocation: %d\n", new_boot_table.blocks_per_allocation);
    dbgprintf("reserved_blocks: %d\n", new_boot_table.reserved_blocks);
    dbgprintf("num_FATs: %d\n", new_boot_table.num_FATs);
    dbgprintf("root_dir_entries: %d\n", new_boot_table.root_dir_entries);
    dbgprintf("total_blocks: %d\n", new_boot_table.total_blocks);
    dbgprintf("media_descriptor: %d\n", new_boot_table.media_descriptor);
    dbgprintf("fat_blocks: %d\n", new_boot_table.fat_blocks);
    dbgprintf("file_system_identifier: %s\n", new_boot_table.file_system_identifier);

    /* Update the boot table */
    boot_table = new_boot_table;

    /* Write the boot table to the boot block */
    write_block((byte_t* )&new_boot_table, BOOT_BLOCK);

    /* Clear out the FAT tables */
    byte_t zero_block[512];
    memset(zero_block, 0, sizeof(zero_block));
    for (uint16_t i = 0; i < new_boot_table.fat_blocks; i++) {
        write_block((byte_t*) zero_block, get_fat_start_block() + i);
        /* We don't need the line for FAT2 anymore. */
    }

    /* Clear out the root directory area */
    for (uint16_t i = 0; i < new_boot_table.root_dir_entries * 32 / 512; i++) {
        write_block(zero_block, get_root_directory_start_block() + i);
    }

    /* Load FAT table into memory. */
    fat_table_memory = (byte_t*)kalloc((boot_table.fat_blocks * 512));  /* Allocate memory for the FAT table */
    for (uint16_t i = 0; i < boot_table.fat_blocks; i++) {
        read_block(fat_table_memory + i * 512, get_fat_start_block() + i);
    }

    return 0;  /* assume success */
}

/* Function to print a root directory entry */
static void fat16_print_directory_entry(const struct fat16_directory_entry *entry)
{
    if (entry->filename[0] == 0x00) { 
        /* Entry is unused */
        return;
    }

    /* Print the filename and extension together. handling the special case for the 0x05 character */
    if (entry->filename[0] == 0x05) {
        dbgprintf("\xE5");  /* Print the 0xE5 character */
        for (int i = 1; i < 8 && entry->filename[i] != ' '; i++) {
            dbgprintf("%c", entry->filename[i]);
        }
    } else {
        for (int i = 0; i < 8 && entry->filename[i] != ' '; i++) {
            dbgprintf("%c", entry->filename[i]);
        }
    }

    /* Print the file extension */
    if (entry->extension[0] != ' ') {
        dbgprintf(".");
        for (int i = 0; i < 3 && entry->extension[i] != ' '; i++) {
            dbgprintf("%c", entry->extension[i]);
        }
    }

    /* Print if it's a directory */
    if (entry->attributes & 0x10) {
        dbgprintf(" <DIR>");
    }

    dbgprintf("\n");
}

void fat16_print_root_directory(struct fat16_directory_entry* buffer)
{
    uint16_t root_directory_blocks = (boot_table.root_dir_entries + ENTRIES_PER_BLOCK - 1) / ENTRIES_PER_BLOCK;
    uint16_t total_entries = root_directory_blocks * ENTRIES_PER_BLOCK;

    dbgprintf("Root Directory:\n");
    for (uint16_t i = 0; i < total_entries; i++) {
        fat16_print_directory_entry(&buffer[i]);
    }
}

void print_root_directory() {
    /* Allocate memory for root directory entries buffer */
    uint16_t root_directory_blocks = (boot_table.root_dir_entries + ENTRIES_PER_BLOCK - 1) / ENTRIES_PER_BLOCK;
    struct fat16_directory_entry* buffer = (struct fat16_directory_entry*)kalloc(root_directory_blocks * 512);

    if (buffer == NULL) {
        dbgprintf("Error: Unable to allocate memory for root directory buffer.\n");
        return;
    }

    /* Read the root directory entries into the buffer */
    fat16_read_root_directory(buffer);

    /* Print the root directory entries from the buffer */
    fat16_print_root_directory(buffer);

    /* Free the allocated memory for the buffer */
    kfree(buffer);
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
    /* load the bootblock */

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