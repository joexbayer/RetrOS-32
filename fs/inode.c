/**
 * @file inode.c
 * @author Joe Bayer (joexbayer)
 * @brief Inode functions.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>

#include <serial.h>

#include <util.h>
#include <diskdev.h>

#define INODE_CACHE_SIZE 100
#define INODE_TO_BLOCK(inode) (INODE_BLOCK(inode))
#define INODE_BLOCK_OFFSET(block, i) ((i-(block*INODES_PER_BLOCK))*sizeof(struct inode))

static struct inode __inode_cache[INODE_CACHE_SIZE];

static void __inode_sync(struct inode* inode, struct superblock* sb)
{
	int inode_int = inode->inode;
	int block_inode = INODE_TO_BLOCK(inode->inode);
	int inode_loc = INODE_BLOCK_OFFSET(block_inode, inode_int);

	if(inode->nlink != 0){
		dbgprintf("[sync] inode %d has %d active links!.\n", inode_int, inode->nlink);
	}

	write_block_offset((char*) inode, sizeof(*inode), inode_loc, sb->inodes_start+block_inode);
	dbgprintf("[sync] Synchronizing inode %d\n", inode_int);
}

static int __inode_cache_insert(struct inode* inode, struct superblock* sb)
{
	int i;
	int lowest_nlink = 9999;

	/* Check if there is a free cache slot and find cache with lowest nlink */
	for (i = 0; i < INODE_CACHE_SIZE; i++){
		if(__inode_cache[i].type == 0){
			dbgprintf("[FS] Caching inode %d.\n", inode->inode);
			memcpy(&__inode_cache[i], inode, sizeof(struct inode));
			return i;
		}
		if(__inode_cache[i].nlink < lowest_nlink){
			lowest_nlink = __inode_cache[i].nlink;
		}
	}

	/* if no free slot check if any file has been closed. */
	for (i = 0; i < INODE_CACHE_SIZE; i++){
		if(__inode_cache[i].nlink == 0){
			__inode_sync(&__inode_cache[i], sb);
			dbgprintf("[FS] Saving inode %d to disk..\n", __inode_cache[i].inode);
			dbgprintf("[FS] Caching inode %d.\n", inode->inode);
			memcpy(&__inode_cache[i], inode, sizeof(struct inode));
			return i;
		}
	}

	/* if all else fails, evict the inode with least nlinks */
	// __inode_sync(&__inode_cache[lowest_cache_entry], sb);
	// dbgprintf("[FS] Saving inode %d to disk..\n", __inode_cache[lowest_cache_entry].inode);
	// dbgprintf("[FS] Caching inode %d.\n", inode->inode);
	// memcpy(&__inode_cache[lowest_cache_entry], inode, sizeof(struct inode));
	dbgprintf("[FS] Cache is full with opened inodes!\n");
	return -1;
}

static int __inode_load(inode_t inode, struct superblock* sb)
{   
	int block_inode = INODE_TO_BLOCK(inode);
	int inode_loc = INODE_BLOCK_OFFSET(block_inode, inode);

	struct inode disk_inode;
	read_block_offset((char*) &disk_inode, sizeof(disk_inode), inode_loc, sb->inodes_start+block_inode);

	dbgprintf("[FS] Loaded inode %d from disk. block: %d, inode_loc: %d\n", disk_inode.inode, sb->inodes_start+block_inode, inode_loc);
	mutex_init(&disk_inode.lock);

	disk_inode.nlink = 0;

	return __inode_cache_insert(&disk_inode, sb);
    
}

static inline int __new_inode(struct superblock* sb)
{
	return get_free_bitmap(sb->inode_map, sb->ninodes)+1;
}

static inline int __new_block(struct superblock* sb)
{
	return get_free_bitmap(sb->block_map, sb->nblocks)+1;
}

void inodes_sync(struct superblock* sb)
{
	for (int i = 0; i < INODE_CACHE_SIZE; i++)
		if(__inode_cache[i].type != 0)
			__inode_sync(&__inode_cache[i], sb);
}

struct inode* inode_get(inode_t inode, struct superblock* sb)
{
	for (int i = 0; i < INODE_CACHE_SIZE; i++){
		if(__inode_cache[i].inode == inode){
			return &__inode_cache[i];
		}
	}

	int ret = __inode_load(inode, sb);
	if(ret >= 0){
		return &__inode_cache[ret];
	}
	return NULL;
}

/**
 * @brief Reads specified amounts of bytes from inode into given buffer.
 * Needs superblock to specify which "fs".
 * @param buf Buffer to copy data into
 * @param size Size of data to copy
 * @param inode file inode
 * @param sb superblock of fs
 * @return int amount of bytes (< 0 on error.)
 */
int inode_read(void* buf, int size, struct inode* inode, struct superblock* sb)
{
	if(inode->pos >= inode->size){
		dbgprintf("Trying to read past end of file.\n");
		return 0;
	}

	dbgprintf("Reading %d from inode: %d (%d)\n", size, inode->size, inode->pos);

	int left = size > inode->size ? inode->size : size;
	int new_pos = inode->pos % BLOCK_SIZE;
	int block = (inode->pos) / BLOCK_SIZE;
	int progress = 0;
	char* buffer = (char*)buf;

	if(size > MAX_FILE_SIZE)
		return -1;

	acquire(&inode->lock);

	/* If we will read past a block "border", only read the missing part of current block. */   
	if(new_pos != 0 && new_pos + size > BLOCK_SIZE){
		int to_read = BLOCK_SIZE - new_pos;
		read_block_offset(&buffer[progress], to_read, new_pos, sb->blocks_start+inode->blocks[block]);
		inode->pos += to_read;
		left -= to_read;
		progress += to_read;

	}

	/* Read entire blocks while possible. */
	while (left > BLOCK_SIZE)
	{
		block = (inode->pos) / BLOCK_SIZE;
		if(inode->blocks[block] == 0){
			release(&inode->lock);
			return -2;
		}
		read_block_offset(&buffer[progress], BLOCK_SIZE, 0, sb->blocks_start+inode->blocks[block]);
		left -= BLOCK_SIZE;
		inode->pos += BLOCK_SIZE;
		progress += BLOCK_SIZE;
	}

	block = (inode->pos) / BLOCK_SIZE;
	read_block_offset(&buffer[progress], left, inode->pos % BLOCK_SIZE, sb->blocks_start+inode->blocks[block]);
	inode->pos += size;

	release(&inode->lock);
	return inode->size > size ? size : inode->size;
}

/**
 * @brief Writes specified amount of bytes from given buffer into inode.
 * Needs superblock to specify which "fs".
 * @param buf Buffer to write from
 * @param size amount of bytes to write
 * @param inode inode of file
 * @param sb superblock of fs
 * @return int amount of bytes written.
 */
int inode_write(void* buf, int size, struct inode* inode, struct superblock* sb)
{

	int original_size = size;
	int new_pos = inode->pos % BLOCK_SIZE;
	int progress = 0;
	int block = (inode->pos) / BLOCK_SIZE;
	char* buffer = (char*)buf;

	if((size + inode->pos) > MAX_FILE_SIZE || block > NDIRECT)
		return -1; /* TODO: FILE OUT OF SPACE ERROR. */

	if(inode->blocks[block] == 0)
		inode->blocks[block] = __new_block(sb);

	/* if pos is 0, we want to rewrite file. TODO: Free not used blocks */
	if(inode->pos == 0) inode->size = 0;

	acquire(&inode->lock);

	/* If we will write past a block "border", only write the missing part of current block. */    
	if(new_pos != 0 && new_pos + size > BLOCK_SIZE){
		int to_write = BLOCK_SIZE - new_pos;
		write_block_offset(&buffer[progress], BLOCK_SIZE - new_pos, new_pos, sb->blocks_start+inode->blocks[block]);
		inode->pos += to_write;
		size -= to_write;
		progress += to_write;
	}

	/* Recalculate block */
	block = (inode->pos) / BLOCK_SIZE;
	if(inode->blocks[block] == 0)
		inode->blocks[block] = __new_block(sb);

	/* While size is greater than block size keep writing blocks */
	while (size > BLOCK_SIZE)
	{
		write_block(&buffer[progress], sb->blocks_start+inode->blocks[block]);
		inode->pos += BLOCK_SIZE;
		size -= BLOCK_SIZE;
		progress += BLOCK_SIZE;

		block = (inode->pos) / BLOCK_SIZE;
		if(inode->blocks[block] == 0)
			inode->blocks[block] = __new_block(sb);
	}

	write_block_offset(&buffer[progress], size, inode->pos % BLOCK_SIZE, sb->blocks_start+inode->blocks[block]);
	inode->pos += size;
	inode->size += original_size;

	__inode_sync(inode, sb);
	release(&inode->lock);

	return original_size;
}

inode_t alloc_inode(struct superblock* sb, char TYPE)
{
	if(TYPE != FS_TYPE_FILE && TYPE != FS_TYPE_DIRECTORY)
		return -1;

	inode_t inode = __new_inode(sb);

	struct inode inode_disk = {
		.inode = inode,
		.size = 0,
		.type = TYPE,
		.pos = 0,
	};
	get_current_time(&inode_disk.time);
	mutex_init(&inode_disk.lock);

	int ret = __inode_cache_insert(&inode_disk, sb);
	if(ret == -1){
		dbgprintf("[FS] Cache is full with opened inodes!\n");
		return 0;
	}

	return inode;
}