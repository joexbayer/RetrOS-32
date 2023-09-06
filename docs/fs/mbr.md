# The MBR (Master Boot Record) 
# =============================

The MBR is the first sector of a disk which contains the partition table.
Parsing the partition is the first step in loading a file system, for each partition
we try to load the file system and if it succeeds we mount it.

## MBR structure
```c
struct mbr_partition_entry {
    uint8_t  status;                 /* 1 byte - bootable status */
    uint8_t  chs_start[3];           /* 3 bytes - CHS start address */
    uint8_t  type;                   /* 1 byte - partition type */
    uint8_t  chs_end[3];             /* 3 bytes - CHS end address */
    uint32_t lba_start;              /* 4 bytes - LBA start address */
    uint32_t num_sectors;            /* 4 bytes - Number of sectors */
}__attribute__((packed));

struct mbr {
    uint8_t  bootstrap[446];         /* 446 bytes - Bootstrap code */
    struct mbr_partition_entry part[4]; /* 64 bytes - Partition entries */
    uint16_t signature;              /* 2 bytes - Signature */
}__attribute__((packed));
```