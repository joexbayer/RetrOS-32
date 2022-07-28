#ifndef __inode_h
#define __inode_h

#include <stdint.h>

#define FILE 0
#define DIRECTORY 1

#define MAX_FILE_SIZE 1024
#define NDIRECT MAX_FILE_SIZE / 512

struct inode {
    uint8_t type;
    uint8_t nlink;          // Number of links to inode in file system
    uint16_t size;            // Size of file (bytes)
    uint8_t blocks[NDIRECT];   // Data block addresses
};

#define INODES_PER_BLOCK (512 / sizeof(struct inode))
#define INODE_BLOCK(i) ((i) / INODES_PER_BLOCK)

#endif /* __inode_h */
