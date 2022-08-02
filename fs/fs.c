#include <fs/fs.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/directory.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>

#include <util.h>

#define FS_START_LOCATION 100
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

static struct superblock superblock;
static struct inode root;
static struct inode current;

int init_fs()
{
    /* Read superblock and check magic. */
    read_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    if(superblock.magic != MAGIC){
        twriteln("FS: No superblock found.");
        mkfs();
        return 1;
    }
}

void __superblock_sync()
{
    write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
    write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);
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

    __superblock_sync();

    inode_t root_inode = alloc_inode(&superblock, FS_DIRECTORY);
    struct inode* root = inode_get(root_inode);

    struct directory_entry self = {
        .inode = root_inode,
        .name = "."
    };

    struct directory_entry back = {
        .inode = root_inode,
        .name = ".."
    };

    inode_write(&back, sizeof(struct directory_entry), root, &superblock);
    inode_write(&self, sizeof(struct directory_entry), root, &superblock);
}

void ls(char* path)
{

}