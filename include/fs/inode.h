#ifndef __inode_h
#define __inode_h

#include <stdint.h>

#define FS_FILE 1
#define FS_DIRECTORY 2

#define MAX_FILE_SIZE 1024
#define NDIRECT MAX_FILE_SIZE / 512

typedef uint16_t inode_t;

struct inode {
    inode_t inode;
    uint8_t type;
    uint8_t nlink;          // Number of links to inode in file system
    uint16_t size;            // Size of file (bytes)
    uint8_t blocks[NDIRECT];   // Data block addresses

    uint16_t pos;
};

#define INODES_PER_BLOCK (512 / sizeof(struct inode))
#define INODE_BLOCK(i) ((i) / INODES_PER_BLOCK)

#include <fs/superblock.h>

int inode_write(char* buf, int size, struct inode* inode, struct superblock* sb);
int inode_read(char* buf, int size, struct inode* inode, struct superblock* sb);
void ls(char* path);

inode_t alloc_inode(struct superblock* sb, char TYPE);
struct inode* inode_get(inode_t inode, struct superblock* sb);

void inodes_sync(struct superblock* sb);
void sync();

#endif /* __inode_h */
