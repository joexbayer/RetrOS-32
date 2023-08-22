#include <stdint.h>

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

#define FAT_BLOCKS (65536*sizeof(uint16_t)/512)

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

#define FAT_BOOT_TABLE_SIZE 64

struct fat_boot_table {
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
};

struct fat16_directory_entry {
    uint8_t filename[8];                /* 8 bytes - Filename */
    uint8_t extension[3];               /* 3 bytes - Extension */
    uint8_t attributes;                 /* 1 byte - Attributes */
    uint8_t reserved;                   /* 1 byte - Reserved */
    uint8_t created_time_tenths;        /* 1 byte - Created time (tenths of a second) */
    uint16_t created_time;              /* 2 bytes - Created time */
    uint16_t created_date;              /* 2 bytes - Created date */
    uint16_t last_access_date;          /* 2 bytes - Last access date */
    uint16_t first_cluster;             /* 2 bytes - Cluster number */
    uint16_t modified_time;             /* 2 bytes - Modified time */
    uint16_t modified_date;             /* 2 bytes - Modified date */
    uint32_t file_size;                 /* 4 bytes - File size in bytes */
};

/* internal fat16 utility functions */
uint16_t get_fat_start_block(void);
uint16_t get_root_directory_start_block(void);
uint16_t fat16_get_fat_entry(uint32_t cluster);
void fat16_set_fat_entry(uint32_t cluster, uint16_t value);
void fat16_sync_fat_table(void);
void fat16_allocate_cluster(uint32_t cluster);
void fat16_free_cluster(uint32_t cluster);
uint32_t fat16_get_free_cluster(void);

int fat16_read(struct fat16_directory_entry* entry, uint32_t start_offset, void* _buffer, int buffer_length);
int fat16_write(struct fat16_directory_entry* entry, int offset, void* data, int data_length);

void fat16_set_time(uint16_t *time, uint8_t hours, uint8_t minutes, uint8_t seconds);
void fat16_set_date(uint16_t *date, uint16_t year, uint8_t month, uint8_t day);

/* Initialize the file system. Returns 0 on success, and a negative value on error. */
int fat16_initialize();

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