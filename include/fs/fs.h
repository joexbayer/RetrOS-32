#ifndef __FS_H
#define __FS_H

#define FS_START_OFFSET

#include <fs/inode.h>

int init_fs();
void mkfs();

inode_t open(char* name);
void file_close(inode_t inode);
void read(inode_t i);

#endif /* __FS_H */
