#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>

#include <util.h>
#include <diskdev.h>

#define INODE_CACHE_SIZE 10
#define INODE_TO_BLOCK(inode) (INODE_BLOCK(inode)+103) // TODO: REPLACE 103
#define INODE_BLOCK_OFFSET(block, inode) ((inode-(block*INODES_PER_BLOCK))*sizeof(struct inode))

static struct inode __inode_cache[INODE_CACHE_SIZE];

static void __inode_sync(struct inode* inode)
{
    
}

static void __inode_cache_insert(struct inode* inode)
{
    for (size_t i = 0; i < 10; i++)
        if(__inode_cache[i].size == 0){
            memcpy(&__inode_cache[i], inode, sizeof(struct inode));
            return;
        }

    /* REPLACE OLD INODE. */
}

static inline int new_inode(struct superblock* sb)
{
    return get_free_bitmap(sb->inode_map, sb->ninodes);
}

static inline int new_block(struct superblock* sb)
{
    return get_free_bitmap(sb->block_map, sb->nblocks)+1;
}

struct inode* inode_get(inode_t inode)
{
    for (int i = 0; i < INODE_CACHE_SIZE; i++)
        if(__inode_cache[i].inode == inode){
            return &__inode_cache[i];
        }

    /* Load inode from disk. */
    return NULL;
}

int inode_add_directory_entry(struct directory_entry* entry, struct inode* inode, struct superblock* sb)
{
    return -1;
}

int inode_read(char* buf, int size, struct inode* inode, struct superblock* sb)
{
    if(size > MAX_FILE_SIZE || size > inode->size || (size + inode->pos) > inode->size)
        return -1;
    
    int block = (size+inode->pos) / BLOCK_SIZE;
    if(inode->blocks[block] == 0)
        return -1;
    
    read_block_offset(buf, size, inode->pos, 103+sb->ninodes+inode->blocks[block]);
    inode->pos += size;

    return size;
}

int inode_write(char* buf, int size, struct inode* inode, struct superblock* sb)
{
    if((size + inode->pos) > MAX_FILE_SIZE)
        return -1; /* TODO: FILE OUT OF SPACE ERROR. */

    if(size > BLOCK_SIZE)
        return -1; /* NOT IMPLEMENTED.*/

    int block = (size+inode->pos) / BLOCK_SIZE;
    if(inode->blocks[block] == 0)
        inode->blocks[block] = new_block(sb);
    
    write_block_offset(buf, size, inode->pos % BLOCK_SIZE, 103+sb->ninodes+inode->blocks[block]);
    inode->pos += size;
    inode->size += size;

    /*size is bigger than 512*/

    /* size + pos > 512 easy*/

    /* size + pos < 512 bad*/

    /**/

    return size;
}

inode_t alloc_inode(struct superblock* sb, char TYPE)
{
    if(TYPE != FS_FILE && TYPE != FS_DIRECTORY)
        return -1;
    
    inode_t inode = new_inode(sb);

    struct inode inode_disk = {
        .inode = inode,
        .size = 0,
        .type = TYPE
    };

    __inode_cache_insert(&inode_disk);

    twritef("%d\n", inode_disk.size);

    return inode;
}