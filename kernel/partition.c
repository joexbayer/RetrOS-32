#include <mbr.h>
#include <fs/fs.h>
#include <kutils.h>
#include <util.h>
#include <serial.h>

#define BOOT_BLOCK 0
#define BOOT_BLOCK_MEMORY 0x7c00

static struct mbr* mbr;

/**
 * @brief Parses the partition table and loads the filesystems.
 * 
 * @return int 
 */
int mbr_partitions_parse()
{
    if(mbr == NULL){
        return -1;
    }

    dbgprintf("Partitions : boot.  type.   lbs_start.    sectors  \n");
    for(int i = 0; i < 4; i++){
        if(mbr->part[i].type == 0) continue;

        switch (mbr->part[i].type){
        case MBR_TYPE_FAT16_LBA:
            fat16_init();
            break;
        default:
            dbgprintf("Unsupported partition type: %d\n", mbr->part[i].type);
            break;
        }
        
        dbgprintf("Partition %d:   %s    0x%x,       %d,         %d\n", i, mbr->part[i].status == MBR_STATUS_ACTIVE ? "*": " ",  mbr->part[i].type, mbr->part[i].lba_start, mbr->part[i].num_sectors);
    }

    return 0;
}

/**
 * @brief Loads the MBR from the boot block.
 * 
 * @return int 
 */
int mbr_partition_load()
{
    mbr = (struct mbr*)BOOT_BLOCK_MEMORY;
    if(mbr->signature != 0xaa55){
        return -1;
    }

    mbr_partitions_parse();

    return 0;

}