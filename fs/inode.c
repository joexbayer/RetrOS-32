#include <fs/inode.h>
#include <fs/superblock.h>

#define INODE_CACHE_SIZE 10
#define INODE_TO_BLOCK(inode) (INODE_BLOCK(inode)+103) // TODO: REPLACE 103
#define INODE_BLOCK_OFFSET(block, inode) ((inode-(block*INODES_PER_BLOCK))*sizeof(struct inode))

static struct inode __inode_cache[INODE_CACHE_SIZE];

static inline int new_inode(struct superblock* sb)
{
    return get_free_bitmap(sb->inode_map, sb->ninodes);
}

int alloc_inode(struct superblock* sb)
{
    inode_t inode = new_inode(sb);
}