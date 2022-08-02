#ifndef __SUPERBLOCK_H
#define __SUPERBLOCK_H

#include <stdint.h>
#include <bitmap.h>

#define BLOCK_SIZE 512
#define MAGIC 0xfeed

struct superblock;

#include <fs/inode.h>

struct superblock {
    uint16_t magic;
    uint32_t size;
    uint32_t nblocks;
    uint32_t ninodes;
    bitmap_t inode_map;
    bitmap_t block_map;

    inode_t root_inode;
};
/*

    File System Layout.
    | Superblock | inode bitmap | block bitmap | x inodes | x blocks | 
    

*/

#endif /* __SUPERBLOCK_H */
