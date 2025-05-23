#ifndef _FAT16_H
#define _FAT16_H

#include <stdint.h>
#include <mbr.h>

/**
 * @brief FAT16 Filesystem
 * DISK::
 * | Boot Sector | FAT1 | Root Directory | Data Clusters |
 * 
 * FAT:
 * | Entry 0 | Entry 1 | Entry 2 | ... | Entry N |
 */

#define BOOT_BLOCK 0
#define ENTRIES_PER_BLOCK (512 / sizeof(struct fat16_directory_entry))

#define GET_DIRECTORY_BLOCK(cluster) (cluster == 0 ? get_root_directory_start_block() : get_data_start_block() + cluster)  
#define IS_DIRECTORY(entry) (entry.attributes & FAT16_FLAG_SUBDIRECTORY)
#define FAT_BLOCKS (65536*sizeof(uint16_t)/512)

#define FAT_BOOT_TABLE_SIZE 64

typedef enum {
    FAT16_FLAG_READ_ONLY = 1 << 0,         /* Indicates that the file is read-only */
    FAT16_FLAG_HIDDEN = 1 << 1,            /* Indicates a hidden file */
    FAT16_FLAG_SYSTEM = 1 << 2,            /* Indicates a system file */
    FAT16_FLAG_VOLUME_LABEL = 1 << 3,      /* Indicates a special entry containing the volume label */
    FAT16_FLAG_SUBDIRECTORY = 1 << 4,      /* Indicates that the entry describes a subdirectory */
    FAT16_FLAG_ARCHIVE = 1 << 5,           /* Archive flag */
    FAT16_FLAG_UNUSED1 = 1 << 6,           /* Not used; must be set to 0 */
    FAT16_FLAG_UNUSED2 = 1 << 7            /* Not used; must be set to 0 */
} fat16_flag_t;

struct fat_boot_table {
    uint8_t jmp[3];                     /* 3 bytes - Jump instruction */
    uint8_t manufacturer[8];            /* 8 bytes - Optional manufacturer description */
    uint16_t bytes_per_plock;           /* 2 bytes - Number of bytes per block */
    uint8_t blocks_per_allocation;      /* 1 byte - Number of blocks per allocation unit */
    uint16_t reserved_blocks;           /* 2 bytes - Number of reserved blocks */
    uint8_t num_FATs;                   /* 1 byte - Number of File Allocation Tables */
    uint16_t root_dir_entries;          /* 2 bytes - Number of root directory entries */
    uint16_t total_blocks;              /* 2 bytes - Total number of blocks in the entire disk (if <= 65535) */
    uint8_t media_descriptor;           /* 1 byte - Media Descriptor */
    uint16_t fat_blocks;                /* 2 bytes - Number of blocks occupied by one copy of the FAT */
    uint16_t blocks_per_track;          /* 2 bytes - Number of blocks per track */
    uint16_t num_heads;                 /* 2 bytes - Number of heads (disk surfaces) */
    uint32_t hidden_blocks;             /* 4 bytes - Number of hidden blocks */
    uint32_t total_blocks_extended;     /* 4 bytes - Total number of blocks in the entire disk (if > 65535) */
    uint16_t drive_number;              /* 2 bytes - Physical drive number */
    uint8_t extended_signature;         /* 1 byte - Extended Boot Record Signature */
    uint32_t volume_serial_number;      /* 4 bytes - Volume Serial Number */
    uint8_t volume_label[11];           /* 11 bytes - Volume Label */
    uint8_t file_system_identifier[8];  /* 8 bytes - File system identifier */
    uint8_t reserved[448];
    uint16_t boot_signature;            /* 2 bytes - Boot signature */
}__attribute__((packed));

struct fat16_directory_entry {
    union {
        struct {
            uint8_t filename[8];
            uint8_t extension[3];
        };
        uint8_t full_name[11];
    };
    uint8_t attributes;                 /* 1 byte - File attributes */
    uint8_t reserved[10];               /* 10 bytes - Reserved for use by Windows NT */
    uint16_t created_time;              /* 2 bytes - Time file was created */
    uint16_t created_date;              /* 2 bytes - Date file was created */
    uint16_t first_cluster;             /* 2 bytes - First cluster in the file's FAT chain */
    uint32_t file_size;                 /* 4 bytes - File size in bytes */
} __attribute__((packed));

struct fat16_file_identifier {
    int16_t directory;
    int16_t index;
};

/* internal fat16 utility functions */
uint16_t get_fat_start_block(void);
uint16_t get_root_directory_start_block(void);
uint16_t get_data_start_block();
uint16_t fat16_get_fat_entry(uint32_t cluster);
void fat16_set_fat_entry(uint32_t cluster, uint16_t value);
void fat16_sync_fat_table(void);
void fat16_allocate_cluster(uint32_t cluster);
void fat16_free_cluster(uint32_t cluster);
uint32_t fat16_get_free_cluster(void);
int fat16_delete_entry(int block, int index);
int fat16_rename_entry(int directory, int index, char* name);

int fat16_used_blocks();

int fat16_read_data(int first_cluster, uint32_t start_offset, void* _buffer, int buffer_length, uint32_t max_length);
int fat16_write_data(int first_cluster, int offset, void* data, int data_length);

void fat16_set_time(uint16_t *time, uint8_t hours, uint8_t minutes, uint8_t seconds);
void fat16_set_date(uint16_t *date, uint16_t year, uint8_t month, uint8_t day);

int fat16_read_entry(uint32_t block, uint32_t index, struct fat16_directory_entry* entry_out);
int fat16_sync_directory_entry(uint16_t block, uint32_t index, const struct fat16_directory_entry* entry);
struct fat16_file_identifier fat16_get_directory_entry(char* path, struct fat16_directory_entry* entry_out);
int fat16_create_directory(const char *name);

int fat16_create_empty_file(const char* path, int directory);
int fat16_create_file(const char *filename, const char* ext, void *data, int data_length);

int fat16_init();

int fat16_name_compare(uint8_t* path_part, uint8_t* full_name);

/* Initialize the file system. Returns 0 on success, and a negative value on error. */
int fat16_load();

/* format the current block device */
int fat16_format(char* label, int reserved);

/* List the contents of a directory. */
int fat16_listdir(const char *path, void (*callback)(const char *name, int is_directory));

#endif  