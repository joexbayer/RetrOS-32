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
#include <util.h>
#include <stdint.h>
#include <fs/fat16.h>
#include <kutils.h>
#include <diskdev.h>
#include <memory.h>
#include <math.h>
#include <sync.h>

#define DIRECTORY_ROOT 0
#define GET_DIRECTORY_BLOCK(cluster) (cluster == 0 ? get_root_directory_start_block() : get_data_start_block() + cluster)  

static struct fat_boot_table boot_table = {0};
static byte_t* fat_table_memory = NULL;  /* pointer to the in-memory FAT table */

/* Temporary "current" directory */
static uint16_t current_dir_block = 0;

/* locks for read / write and management */
static mutex_t fat16_table_lock; 
static mutex_t fat16_write_lock;
static mutex_t fat16_management_lock;

static struct fat16_directory_entry root_directory = {
    .filename = "ROOT    ",
    .extension = "   ",
    .attributes = FAT16_FLAG_SUBDIRECTORY,
    .first_cluster = 0,
    .file_size = 0
};

/* HELPER FUNCTIONS */

inline uint16_t get_fat_start_block()
{
    return BOOT_BLOCK + boot_table.reserved_blocks;  /* FAT starts right after the boot block and reserved blocks. */
}

inline uint16_t get_root_directory_start_block()
{
    return get_fat_start_block() + boot_table.fat_blocks;  /* Root directory starts after FAT */
}

inline uint16_t get_data_start_block()
{
    return get_root_directory_start_block()-1;  /* Data starts after Root directory, -1 for unused fat table entries. */
}


uint16_t fat16_get_fat_entry(uint32_t cluster)
{
    if(fat_table_memory == NULL){
        return -1;
    }

    uint32_t fat_offset = cluster * 2;  /* Each entry is 2 bytes */
    return *(uint16_t*)(fat_table_memory + fat_offset);
}

void fat16_set_fat_entry(uint32_t cluster, uint16_t value)
{
    if(fat_table_memory == NULL){
        return;
    }

    acquire(&fat16_table_lock);

    uint32_t fat_offset = cluster * 2;  /* Each entry is 2 bytes */
    *(uint16_t*)(fat_table_memory + fat_offset) = value;

    release(&fat16_table_lock);
}

void fat16_sync_fat_table()
{
    if(fat_table_memory == NULL){
        return;
    }

    acquire(&fat16_table_lock);

    int start_block = get_fat_start_block();
    for (uint16_t i = 0; i < boot_table.fat_blocks; i++) {
        write_block(fat_table_memory + i * 512, start_block + i);
    }

    release(&fat16_table_lock);
}

/* wrapper functions TODO: inline replace */
inline void fat16_allocate_cluster(uint32_t cluster)
{
    fat16_set_fat_entry(cluster, 0xFFFF);  /* marking cluster as end of file */
}

inline void fat16_free_cluster(uint32_t cluster)
{
    fat16_set_fat_entry(cluster, 0x0000);  /* marking cluster as free */
}

uint32_t fat16_get_free_cluster()
{
    acquire(&fat16_table_lock);

    for (int i = 0; i < 65536; i++) {  /* Start from 2 as 0 and 1 are reserved entries */
        if (fat16_get_fat_entry(i) == 0x0000) {

            fat16_allocate_cluster(i);

            release(&fat16_table_lock);
            return i;
        }
    }

    release(&fat16_table_lock);
    return -1;  /* no free cluster found */
}

/**
 * Writes (synchronizes) a directory entry back to the root directory of the FAT16 filesystem.
 *
 * @param index: The index of the directory entry to be updated.
 * @param entry: Pointer to the directory entry data to be written.
 * @return 0 on success, negative value on error.
 */
int fat16_sync_directory_entry(uint16_t block, uint32_t index, const struct fat16_directory_entry* entry)
{
    if(index >= boot_table.root_dir_entries)
        return -1;  /* index out of range */

    uint32_t offset = (index % ENTRIES_PER_BLOCK) * sizeof(struct fat16_directory_entry);
    write_block_offset((byte_t*)entry, sizeof(struct fat16_directory_entry), offset, block);

    return 0;  /* success */
}

int fat16_read_entry(uint32_t block, uint32_t index, struct fat16_directory_entry* entry_out)
{
    if(index >= ENTRIES_PER_BLOCK)
        return -1;  /* index out of range */

    byte_t buffer[512];
    if(read_block(buffer, block) < 0){
        dbgprintf("Error reading block\n");
        return -2;  /* error reading block */
    }

    struct fat16_directory_entry* dir_entry = (struct fat16_directory_entry*)(buffer + (index * sizeof(struct fat16_directory_entry)));

    /* Copy the directory entry to the output */
    memcpy(entry_out, dir_entry, sizeof(struct fat16_directory_entry));
    
    return 0;  /* success */
}

/**
 * @brief 
 * 
 * @param filename 
 * @param ext 
 * @param entry_out 
 * @return int index of directory
 */
static int fat16_find_entry(const char *filename, const char* ext, struct fat16_directory_entry* entry_out)
{
    /* Search the root directory for the file. */
    for (int i = 0; i < boot_table.root_dir_entries; i++) {
        struct fat16_directory_entry entry = {0};

        fat16_read_entry(current_dir_block ,i, &entry);
        if (memcmp(entry.filename, filename, strlen(filename)) == 0 && memcmp(entry.extension, ext, 3) == 0) {
            if (entry_out) {
                *entry_out = entry;
            }
            dbgprintf("Found file at index %d\n", i);

            /* print file info */
            dbgprintf("Filename: %s.%s (%d bytes) Attributes: 0x%x Cluster: %d %s\n", entry.filename, entry.extension, entry.file_size, entry.attributes, entry.first_cluster, entry.attributes & 0x10 ? "<DIR>" : "");
            return i;  /* Found */
        }
    }
    return -1;  /* Not found */
}

/**
 * Adds a new root directory entry.
 * @param filename The name of the file (up to 8 characters, not including the extension).
 * @param extension The file extension (up to 3 characters).
 * @param attributes Attributes for the file (read-only, hidden, system, etc.).
 * @param start_cluster The first cluster of the file.
 * @param file_size The size of the file in bytes.
 * @return 0 on success, or a negative value on error.
 */
int fat16_add_entry(uint16_t block, char *filename, const char *extension, byte_t attributes, uint16_t start_cluster, uint32_t file_size)
{
    byte_t buffer[512] = {0};

    if(read_block(buffer, block) < 0){
        return -1;  /* error reading block */
    }

    for (int entry = 0; entry < (int)ENTRIES_PER_BLOCK; entry++) {

        struct fat16_directory_entry* dir_entry = (struct fat16_directory_entry*)(buffer + entry * sizeof(struct fat16_directory_entry));
        if (dir_entry->filename[0] == 0x00 || dir_entry->filename[0] == 0xE5) {  /* empty or deleted entry */
            /* Fill in the directory entry */
            memset(dir_entry, 0, sizeof(struct fat16_directory_entry));  /* Clear the entry */
            memset(dir_entry->filename, ' ', 8);  /* Set the filename to spaces */
            memcpy(dir_entry->filename, filename, 8);
            memcpy(dir_entry->extension, extension, 3);
            dir_entry->attributes = attributes;
            dir_entry->first_cluster = start_cluster;
            dir_entry->file_size = file_size;

            uint16_t local_date = dir_entry->created_date;
            fat16_set_date(&local_date, 2023, 5, 31);
            dir_entry->created_date = local_date;

            uint16_t local_time = dir_entry->created_time;
            fat16_set_time(&local_time, 12, 0, 0);
            dir_entry->created_time = local_time;

            dbgprintf("Adding entry %s.%s (%d bytes) Attributes: 0x%x Cluster: %d %s to %d index %d\n", dir_entry->filename, dir_entry->extension, dir_entry->file_size, dir_entry->attributes, dir_entry->first_cluster, dir_entry->attributes & 0x10 ? "<DIR>" : "", block, entry);

            /* Write the block back to disk */
            write_block(buffer, block);
            return 0;  /* success */
        }
    }
    

    return -1;  /* no empty slot found in the root directory */
}

int fat16_name_compare(uint8_t* path_part, uint8_t* full_name)
{
    int i;
    int len = strlen((char*)path_part);

    // Compare filename portion (first 8 characters)
    for (i = 0; i < 8 && i < len; i++) {
        if (path_part[i] != full_name[i]) {
            return 0; // Characters don't match.
        }
    }

    // If we've exhausted the path part, compare the remaining filename characters with space.
    for (; i < 8; i++) {
        if (full_name[i] != ' ') {
            return 0; // They are not the same.
        }
    }

    // If path_part had '.' move past it
    if (path_part[i] == '.') {
        i++;
    }

    // Compare the extension (next 3 characters)
    for (int j = 0; j < 3 && i < len; j++, i++) {
        if (path_part[i] != full_name[j + 8]) {
            return 0; // Characters don't match.
        }
    }

    // If we've exhausted the path part, compare the remaining extension characters with space.
    for (int j = i - 8; j < 3; j++) {
        if (full_name[j + 8] != ' ') {
            return 0; // They are not the same.
        }
    }

    return 1; // The names are the same.
}

char* sstrtok(char* str, const char* delim)
{
    static char local_buffer[128];
    static char* current = NULL;   /* Keeps track of the current position in the string */

    if (str != NULL) {
        memcpy(local_buffer, str, strlen(str));
        local_buffer[strlen(str)+1] = '\0';  // Ensure null-termination
        current = local_buffer;
    }

    if (current == NULL || *current == '\0') {
        return NULL;   /* If there is no more string or the current position is at the end, return NULL */
    }

    char* token = current;   /* Start of the next token */

    /* Find the next occurrence of any delimiter characters */
    while (*current != '\0' && strchr(delim, *current) == NULL) {
        ++current;
    }

    if (*current != '\0') {
        *current = '\0';   /* Null-terminate the token */
        ++current;   /* Move the current position to the next character after the delimiter */
    }


    return token;   /* Return the next token */
}

int fat16_directory_entry_debug(struct fat16_directory_entry* entry)
{
    dbgprintf("Entry (%s):\n", entry->attributes & FAT16_FLAG_SUBDIRECTORY ? "directory" : "file");
    dbgprintf("  Filename: %s\n", entry->filename);
    dbgprintf("  Extension: %s\n", entry->extension);
    dbgprintf("  Attributes: 0x%x\n", entry->attributes);
    dbgprintf("  Reserved: 0x%x\n", entry->reserved);
    dbgprintf("  Created time: 0x%x\n", entry->created_time);
    dbgprintf("  Created date: 0x%x\n", entry->created_date);
    return 0;
}


/**
 * @brief Retrieves a directory entry by path.
 * 
 * @param path The path to the file or directory.
 * @param entry_out Pointer to the destination struct where the entry should be copied.
 * @return directory cluster on success, or a negative value on error.
 */
struct fat16_file_identifier fat16_get_directory_entry(char* path, struct fat16_directory_entry* entry_out)
{
    uint16_t start_block;
    uint32_t index;
    struct fat16_directory_entry entry = {0};

    dbgprintf("Searching for %s\n", path);
    if(path[0] == '/'){
        start_block = get_root_directory_start_block();
        path++;

        if(strlen(path) == 0){
            memcpy(entry_out, &root_directory, sizeof(struct fat16_directory_entry));
            
            struct fat16_file_identifier identifier = {
                .directory = get_root_directory_start_block(),
                .index = 0
            };
            return identifier;
        }

    } else {
        start_block = current_dir_block;
    }

    uint8_t* token = (uint8_t*)sstrtok(path, "/");
    dbgprintf("Token: %s\n", token);
    while (token != NULL) {
        int found = 0;

        //dbgprintf("Searching for %s in %d\n", token, start_block);

        for(index = 0; index < ENTRIES_PER_BLOCK; index++) {
            if(fat16_read_entry(start_block, index, &entry) < 0) continue;
            if (fat16_name_compare(token, entry.full_name)) {
                if (entry.attributes & FAT16_FLAG_SUBDIRECTORY) {
                    start_block = GET_DIRECTORY_BLOCK(entry.first_cluster);
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            return (struct fat16_file_identifier){0};
        }

        token = (uint8_t*)sstrtok(NULL, "/");
    }

    memcpy(entry_out, &entry, sizeof(struct fat16_directory_entry));
    
    return (struct fat16_file_identifier){
        .directory = start_block,
        .index = index
    };
}

int fat16_create_directory(const char* name)
{
    uint16_t start_block = current_dir_block;
    uint16_t directory_block;
    
    if(strlen(name) > 8){
        dbgprintf("Directory name too long\n");
        return -1;
    }

    directory_block = fat16_get_free_cluster();

    dbgprintf("Creating directory %s root: %d, block: %d\n", name, current_dir_block, directory_block);

    fat16_add_entry(start_block, (char*)name, "   ", FAT16_FLAG_SUBDIRECTORY, directory_block, 0);

    /* add .. and . directories */
    fat16_add_entry(get_data_start_block() + directory_block, ".       ", "   ", FAT16_FLAG_SUBDIRECTORY, directory_block, 0);
    
    fat16_add_entry(
        get_data_start_block() + directory_block,
        "..      ", "   ",
        FAT16_FLAG_SUBDIRECTORY,
        start_block == get_root_directory_start_block() ? 0 : start_block - get_data_start_block(),
        0
    );

    fat16_sync_fat_table();

    return 0;
}


int fat16_read_file(const char *filename, const char* ext, void *buffer, int buffer_length)
{
    struct fat16_directory_entry entry;
    int find_result = fat16_find_entry(filename, ext, &entry);
    if (find_result < 0) {
        dbgprintf("File not found\n");
        return -1;
    }  /* File not found */

    int ret = fat16_read_data(entry.first_cluster, 0, buffer, buffer_length, entry.file_size);

    /* sync directory entry? */

    return ret;  /* Return bytes read */
}

int fat16_create_file(const char *filename, const char* ext, void *data, int data_length)
{
    int first_cluster = fat16_get_free_cluster();  
    if (first_cluster < 0) {
        dbgprintf("No free cluster found\n");
        return -1;  /* No free cluster found */
    }

    struct fat16_directory_entry entry = {
        .first_cluster = first_cluster,
    };

    fat16_write_data(entry.first_cluster, 0, data, data_length);


    fat16_add_entry(current_dir_block, (char*)filename, ext, FAT16_FLAG_ARCHIVE, first_cluster, data_length);
    fat16_sync_fat_table();

    return 0;  /* Success */ 
}

void fat16_directory_entries(uint16_t block)
{
    dbgprintf("Directory entries for block %d\n", block);

    for (int i = 0; i < (int)ENTRIES_PER_BLOCK; i++) {
        struct fat16_directory_entry entry = {0};
        struct fat16_directory_entry* dir_entry = &entry;

        fat16_read_entry(block, i, &entry);
        /* Check if the entry is used (filename's first byte is not 0x00 or 0xE5) */
        if (dir_entry->filename[0] != 0x00 && dir_entry->filename[0] != 0xE5) {
            char name[9];
            memcpy(name, dir_entry->filename, 8);
            name[8] = '\0';

            char extension[4];
            memcpy(extension, dir_entry->extension, 3);
            extension[3] = '\0';

            /* print detailed info about the entry */
            dbgprintf("Filename: %s.%s (%d bytes) Attributes: 0x%x Cluster: %d %s\n", name, extension, dir_entry->file_size, dir_entry->attributes, dir_entry->first_cluster, dir_entry->attributes & 0x10 ? "<DIR>" : "");
        }
    }
}

int fat16_mbr_clear()
{
    byte_t mbr[512] = {0};
    if(read_block(mbr, 0) < 0){
        dbgprintf("Error reading block\n");
        return -1;  /* error reading block */
    }

    for(int i = 0; i < 4; i++) {
        struct mbr_partition_entry *entry = (struct mbr_partition_entry *)&mbr[446 + (i * sizeof(struct mbr_partition_entry))];
        memset(entry, 0, sizeof(struct mbr_partition_entry));
    }

    dbgprintf("MBR cleared\n");
    write_block(mbr, 0);
    return 0;
}

int fat16_mbr_add_entry(uint8_t bootable, uint8_t type, uint32_t start, uint32_t size)
{
    byte_t mbr[512] = {0};
    if(read_block(mbr, 0) < 0){
        dbgprintf("Error reading block\n");
        return -1;  /* error reading block */
    }

    for(int i = 0; i < 4; i++) {
        struct mbr_partition_entry *entry = (struct mbr_partition_entry *)&mbr[446 + (i * sizeof(struct mbr_partition_entry))];
        if(entry->type == 0x00) {
            entry->status = bootable;
            entry->type = type;
            entry->lba_start = start;
            entry->num_sectors = size;

            dbgprintf("MBR entry added\n");
            write_block(mbr, 0);
            return 0;
        }
    }

    dbgprintf("No empty slot found in the MBR\n");

    return -1;
}

void fat16_print_root_directory_entries()
{
    fat16_directory_entries(current_dir_block);
}

void fat16_change_directory(const char* name)
{
    struct fat16_directory_entry entry;
    int find_result = fat16_find_entry(name, "   ", &entry);
    if (find_result < 0) {
        dbgprintf("Directory not found\n");
        return;
    }  /* Directory not found */

    if((entry.attributes & 0x10) == 0){
        dbgprintf("Not a directory\n");
        return;
    }

    current_dir_block = get_data_start_block()+ entry.first_cluster;
    dbgprintf("Changed directory to %s\n", name);
}

void fat16_bootblock_info()
{
        /* dbgprint out bootblock information: */
    dbgprintf("bootblock information:\n");
    dbgprintf("manufacturer: %s\n", boot_table.manufacturer);
    dbgprintf("bytes_per_plock: %d\n", boot_table.bytes_per_plock);
    dbgprintf("blocks_per_allocation: %d\n", boot_table.blocks_per_allocation);
    dbgprintf("reserved_blocks: %d\n", boot_table.reserved_blocks);
    dbgprintf("num_FATs: %d\n", boot_table.num_FATs);
    dbgprintf("root_dir_entries: %d\n", boot_table.root_dir_entries);
    dbgprintf("total_blocks: %d\n", boot_table.total_blocks);
    dbgprintf("media_descriptor: %d\n", boot_table.media_descriptor);
    dbgprintf("fat_blocks: %d\n", boot_table.fat_blocks);
    dbgprintf("file_system_identifier: %s\n", boot_table.file_system_identifier);

    dbgprintf("get_fat_start_block: %d\n", get_fat_start_block());
    dbgprintf("get_root_directory_start_block: %d\n", get_root_directory_start_block());
    dbgprintf("get_data_start_block: %d\n", get_data_start_block());
}

/**
 * Formats the disk with the FAT16 filesystem.
 * @param label The volume label (up to 11 characters). (NOT IMPLEMENTED)
 * @param reserved The number of reserved blocks (usually 1).
 * 
 * @warning This will erase all data on the disk.
 * @return 0 on success, or a negative value on error.
 */
int fat16_format(char* label, int reserved)
{
    if(disk_attached() == 0){
        dbgprintf("No disk attached\n");
        return -1;
    }

    int total_blocks = (disk_size()/512)-1;
    dbgprintf("Total blocks: %d (%d/512)\n", total_blocks, disk_size());

    /* read bootblock for potential boot code, then cast fat_boot_table to it and update the talbe */
    byte_t bootblock[512];
    if(read_block(bootblock, BOOT_BLOCK) < 0){
        dbgprintf("Error reading boot block\n");
        return -2;
    }

    struct fat_boot_table* boot_table_ptr = (struct fat_boot_table*)bootblock;
    boot_table_ptr->bytes_per_plock = 512;      /* Standard block size */
    boot_table_ptr->blocks_per_allocation = 1;  /* Usually 1 for small devices */
    boot_table_ptr->reserved_blocks = reserved; /* The boot block, will also include kernel? */
    boot_table_ptr->num_FATs = 1;               /* Standard for FAT16 */
    boot_table_ptr->root_dir_entries = 16;      /* This means the root directory occupies 1 block TODO: Currently hardcoded*/
    boot_table_ptr->total_blocks = total_blocks;
    boot_table_ptr->media_descriptor = 0xF8;    /* Fixed disk  */
    boot_table_ptr->fat_blocks = (total_blocks*sizeof(uint16_t)/512);
    boot_table_ptr->volume_serial_number = 0x12345678;  /* TODO: Randomize */
    boot_table_ptr->extended_signature = 0x29;  /* Extended boot record signature */
    /* ... other fields ... */
    boot_table_ptr->boot_signature = 0xAA55;
    
    memcpy(boot_table_ptr->volume_label, "VOLUME1    ", 11);
    memcpy(boot_table_ptr->file_system_identifier, "FAT16   ", 8); /* This can be any 8-character string */
    memcpy(boot_table_ptr->manufacturer, "NETOS   ", 8); /* This can be any 8-character string */

    /* Update the boot table */
    boot_table = *boot_table_ptr;
    fat16_bootblock_info();

    /* Write the boot table to the boot block */
    if(write_block(bootblock, BOOT_BLOCK) < 0){
        dbgprintf("Error writing boot block\n");
        return -3;
    }

    /* Clear out the FAT tables */
    byte_t zero_block[512];
    memset(zero_block, 0, sizeof(zero_block));
    for (uint16_t i = 0; i < boot_table.fat_blocks; i++) {
        write_block((byte_t*) zero_block, get_fat_start_block() + i);
    }


    /* Clear out the root directory area */
    for (uint16_t i = 0; i < boot_table.root_dir_entries * 32 / 512; i++) {
        write_block(zero_block, get_root_directory_start_block() + i);
    }

    fat16_mbr_clear();

    fat16_mbr_add_entry(MBR_STATUS_ACTIVE, MBR_TYPE_FAT16_LBA, BOOT_BLOCK, total_blocks);

    dbgprintf("FAT16 formatted\n");

    return 0;  /* assume success */
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

int fat16_load()
{
    /* load the bootblock */
    read_block((byte_t*)&boot_table, BOOT_BLOCK);

    /* confirm that bootblock is correct */
    if (memcmp(boot_table.manufacturer, "NETOS   ", 8) != 0) {
        dbgprintf("Bootblock manufacturer is not NETOS\n");
        return -1;
    }

        /* init mutexes */
    mutex_init(&fat16_table_lock);
    mutex_init(&fat16_write_lock);
    mutex_init(&fat16_management_lock);

    fat16_bootblock_info();

    /* Load FAT table into memory. */
    fat_table_memory = (byte_t*)kalloc((boot_table.fat_blocks * 512));  /* Allocate memory for the FAT table */
    for (uint16_t i = 0; i < boot_table.fat_blocks; i++) {
        read_block(fat_table_memory + i * 512, get_fat_start_block() + i);
    }
    fat16_set_fat_entry(0, 0xFF00 | 0xF8); 
    fat16_allocate_cluster(1);
    fat16_add_entry(get_root_directory_start_block(), "VOLUME1 ", "   ", FAT16_FLAG_VOLUME_LABEL, 0, 0);

    current_dir_block = get_root_directory_start_block();

    dbgprintf("FAT16 initialized\n");

    return 0;
}

/* Open a file. Returns a file descriptor or a negative value on error. */
int fat16_open(const char *path);

/* Close an open file. Returns 0 on success, and a negative value on error. */
int fat16_close(int fd);

/* Create a new directory. Returns 0 on success, and a negative value on error. */
int fat16_mkdir(const char *path);

/* Delete a file or directory. Returns 0 on success, and a negative value on error. */
int fat16_remove(const char *path);

/* List the contents of a directory. */
int fat16_listdir(const char *path, void (*callback)(const char *name, int is_directory));
