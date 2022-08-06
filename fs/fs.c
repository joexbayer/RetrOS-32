#include <fs/fs.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/directory.h>
#include <memory.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>

#include <util.h>

#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

static struct superblock superblock;
static struct inode* root_dir;
static struct inode* current_dir;
static int FS_START_LOCATION = 0;

int init_fs()
{
    FS_START_LOCATION = (kernel_size/512)+2;
    /* Read superblock and check magic. */
    read_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    if(superblock.magic != MAGIC){
        twriteln("FS: No superblock found.");
        mkfs();
        return 1;
    }

    twritef("FS: Found Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
    twritef("FS: With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
    twritef("FS: And total of %d block\n", superblock.nblocks);
    twritef("FS: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);

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

static inline void __inode_add_dir(struct directory_entry* entry, struct inode* inode, struct superblock* sb)
{
    inode->pos = inode->size;
    inode_write((char*) entry, sizeof(struct directory_entry), inode, sb);
}

void mkfs()
{
    superblock.magic = MAGIC;
    superblock.size = (disk_size()) - (FS_START_LOCATION*BLOCK_SIZE);

    superblock.ninodes = (superblock.size / (sizeof(struct inode)+NDIRECT*BLOCK_SIZE));
    superblock.nblocks = superblock.ninodes*NDIRECT;

    superblock.inodes_start = FS_START_LOCATION + 3;
    superblock.blocks_start = superblock.inodes_start + (superblock.ninodes/ INODES_PER_BLOCK);

    superblock.block_map = create_bitmap(superblock.nblocks);
    superblock.inode_map = create_bitmap(superblock.ninodes);

    twritef("FS: Creating Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
    twritef("FS: With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
    twritef("FS: And total of %d block\n", superblock.nblocks);
    twritef("FS: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);

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

    __inode_add_dir(&back, current_dir, &superblock);
    __inode_add_dir(&self, current_dir, &superblock);
    __inode_add_dir(&home, current_dir, &superblock);

    current_dir->pos = 0;
}

void create_file(char* name)
{
    inode_t inode = alloc_inode(&superblock, FS_FILE);
}

void file_read(inode_t i)
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

    if(entry.inode == 0)
        twriteln("Cant find");

    struct inode* inode = inode_get(entry.inode, &superblock);
    inode->nlink++;
    inode->pos = 0;
    
    return entry.inode;
}

void chdir(char* path)
{
    inode_t ret = open(path);
    struct inode* inode = inode_get(ret, &superblock);
    if(inode->type != FS_DIRECTORY){
        twritef("%s is not a directory.\n", path);
        return;
    }

    current_dir = inode;
}

void mkdir(char* name)
{
    inode_t inode_index = alloc_inode(&superblock, FS_DIRECTORY);
    struct inode* inode = inode_get(inode_index, &superblock);

    struct directory_entry self = {
        .inode = inode->inode,
        .name = "."
    };

    struct directory_entry back = {
        .inode = current_dir->inode,
        .name = ".."
    };

    struct directory_entry new = {
        .inode = inode->inode
    };
    memcpy(new.name, name, strlen(name));

    __inode_add_dir(&back, inode, &superblock);
    __inode_add_dir(&self, inode, &superblock);
    __inode_add_dir(&new, current_dir, &superblock);
}

void ls(char* path)
{
    struct directory_entry entry;
    int size = 0;

    current_dir->pos = 0;
    while (size < current_dir->size)
    {
        int ret = inode_read((char*) &entry, sizeof(struct directory_entry), current_dir, &superblock);
        twritef(" %s\n", entry.name);
        size += ret;
    }
}