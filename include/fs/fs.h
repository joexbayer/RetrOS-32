#ifndef __FS_H
#define __FS_H

#define FS_START_OFFSET

#include <fs/inode.h>

int init_fs();
void mkfs();

inode_t fs_open(char* name);
void fs_close(inode_t inode);
int fs_read(inode_t i, char* buf, int size);
int fs_write(void* buf, int size, inode_t i);
int chdir(char* path);
int fs_mkdir(char* name);
int fs_create(char* name);

int fs_size(inode_t i);

void fs_stats();

#endif /* __FS_H */
