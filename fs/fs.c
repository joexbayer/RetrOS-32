#include <fs/fs.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/directory.h>
#include <memory.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>

#include <util.h>

#define FS_START_LOCATION 100
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

static struct superblock superblock;
static struct inode* root_dir;
static struct inode* current_dir;

int init_fs()
{
    /* Read superblock and check magic. */
    read_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    if(superblock.magic != MAGIC){
        twriteln("FS: No superblock found.");
        mkfs();
        return 1;
    }

    twritef("FS: Found Filesystem with size: %d\n", superblock.nblocks*512);
    twritef("FS: With a total of %d inodes\n", superblock.ninodes);
    twritef("FS: Max file size: %d bytes\n", NDIRECT*512);

    root_dir = inode_get(superblock.root_inode, &superblock);
    root_dir->nlink++;
    current_dir = root_dir;

    return 0;
}

void __superblock_sync()
{
    write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
    write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);

    twriteln("[sync] Superblock... (DONE)");
}

void sync()
{
    twriteln("[sync] Synchronizing filesystem.");
    __superblock_sync();
    inodes_sync(&superblock);
    twritef("[sync] %d inodes... (DONE)\n", superblock.ninodes);
    twriteln("[sync] Filesystem successfully synchronized to disk!.");
}

void mkfs()
{
    superblock.magic = MAGIC;
    superblock.size = (disk_size()) - (FS_START_LOCATION*512);
    superblock.ninodes = 200;
    superblock.nblocks = superblock.ninodes*NDIRECT;

    superblock.inodes_start = FS_START_LOCATION + 3;
    superblock.blocks_start = superblock.inodes_start + superblock.ninodes;

    superblock.block_map = create_bitmap(superblock.nblocks);
    superblock.inode_map = create_bitmap(superblock.ninodes);

    twritef("FS: Creating Filesystem with size: %d\n", superblock.nblocks*512);
    twritef("FS: With a total of %d inodes\n", superblock.ninodes);
    twritef("FS: Max file size: %d bytes\n", NDIRECT*512);

    inode_t root_inode = alloc_inode(&superblock, FS_DIRECTORY);
    struct inode* root = inode_get(root_inode, &superblock);
    root->nlink++;

    superblock.root_inode = root_inode;

    struct directory_entry self = {
        .inode = root_inode,
        .name = "."
    };

    struct directory_entry back = {
        .inode = root_inode,
        .name = ".."
    };

    inode_t home_inode = alloc_inode(&superblock, FS_FILE);
    struct inode* home_disk_inode = inode_get(home_inode, &superblock);

    struct directory_entry home = {
        .inode = home_inode,
        .name = "home"
    };

    char* home_text = "Home is where the heart is.";
    inode_write(home_text, strlen(home_text)+1, home_disk_inode, &superblock);

    root_dir = root;
    current_dir = root;

    inode_write((char*) &self, sizeof(struct directory_entry), current_dir, &superblock);
    inode_write((char*) &back, sizeof(struct directory_entry), current_dir, &superblock);
    inode_write((char*) &home, sizeof(struct directory_entry), current_dir, &superblock);

    current_dir->pos = 0;
}

void create_file(char* name)
{
    inode_t inode = alloc_inode(&superblock, FS_FILE);
}

void read(inode_t i)
{
    struct inode* inode = inode_get(i, &superblock);
    inode->pos = 0;

    char* value = alloc(inode->size);
    inode_read(value, inode->size, inode, &superblock);
    twritef("%s\n", value);
    free(value);
}

void file_close(inode_t inode)
{
    struct inode* inode_disk = inode_get(inode, &superblock);
    inode_disk->nlink--;
}

inode_t open(char* name)
{
    struct directory_entry entry;
    current_dir->pos = 0;

    int size = 0;
    while (size <= current_dir->size)
    {
        int ret = inode_read((char*) &entry, sizeof(struct directory_entry), current_dir, &superblock);
        if(memcmp((void*)entry.name, (void*)name, strlen(entry.name)))
            break;
        size += ret;
    }

    struct inode* inode = inode_get(entry.inode, &superblock);
    inode->nlink++;
    inode->pos = 0;
    
    return entry.inode;
}

void ls(char* path)
{
    struct directory_entry entry;
    int size = 0;

    root_dir->pos = 0;
    while (size < root_dir->size)
    {
        int ret = inode_read((char*) &entry, sizeof(struct directory_entry), root_dir, &superblock);
        twritef(" %s\n", entry.name);
        size += ret;
    }
}