#ifndef MBR_H
#define MBR_H

#include <stdint.h>

typedef enum {
    MBR_STATUS_INACTIVE = 0x00,  /* Partition is not bootable */
    MBR_STATUS_ACTIVE = 0x80     /* Partition is bootable */
} mbr_partition_status_t;

typedef enum {
    MBR_TYPE_EMPTY = 0x00,              /* Empty or unused partition */
    MBR_TYPE_FAT12 = 0x01,              /* FAT12 filesystem */
    MBR_TYPE_XENIX_ROOT = 0x02,         /* XENIX root filesystem */
    MBR_TYPE_XENIX_USR = 0x03,          /* XENIX /usr filesystem */
    MBR_TYPE_FAT16_LT32M = 0x04,        /* FAT16 filesystem with less than 32MB */
    MBR_TYPE_EXTENDED = 0x05,           /* Extended partition */
    MBR_TYPE_FAT16_GT32M = 0x06,        /* FAT16 filesystem with more than 32MB */
    MBR_TYPE_NTFS = 0x07,               /* NTFS or exFAT filesystem */
    MBR_TYPE_FAT32_CHS = 0x0B,          /* FAT32 filesystem (CHS addressing) */
    MBR_TYPE_FAT32_LBA = 0x0C,          /* FAT32 filesystem (LBA addressing) */
    MBR_TYPE_FAT16_LBA = 0x0E,          /* FAT16 filesystem (LBA addressing) */
    MBR_TYPE_EXTENDED_LBA = 0x0F,       /* Extended partition using LBA */
    MBR_TYPE_LINUX = 0x83,              /* Linux native partition */
} mbr_partition_type_t;


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

int mbr_partitions_parse();
int mbr_partition_load();
struct mbr* mbr_get();

const char* mbr_partition_type_string(mbr_partition_type_t type);

#endif /* MBR_H */