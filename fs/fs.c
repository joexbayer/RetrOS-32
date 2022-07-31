#include <fs/fs.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>

#define FS_START_LOCATION 100

static struct superblock superblock;
static struct inode __inode_cache[10];

int init_fs()
{
    /* Read superblock and check magic. */
    char buf[BLOCK_SIZE];
    disk_device.read(&buf, FS_START_LOCATION, 1);
    memcpy(&superblock, buf, sizeof(struct superblock));
    if(superblock.magic != MAGIC){
        twriteln("FS: No superblock found.");
        mkfs();
    }
}

void mkfs()
{
    superblock.magic = MAGIC;
    superblock.size = (disk_device.dev->size*512) - (FS_START_LOCATION*512);
    superblock.ninodes = 200;
    superblock.nblocks = superblock.ninodes*NDIRECT;

    superblock.block_map = create_bitmap(superblock.nblocks);
    superblock.inode_map = create_bitmap(superblock.ninodes);

    twritef("FS: Creating Filesystem with size: %d\n", superblock.nblocks*512);
    twritef("FS: With a total of %d inodes\n", superblock.ninodes);
    twritef("FS: Max file size: %d bytes\n", NDIRECT*512);
}