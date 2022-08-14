#ifndef __FS_H
#define __FS_H

#define FS_START_OFFSET

#include <fs/inode.h>

int init_fs();
void mkfs();

inode_t fs_open(char* name);
void file_close(inode_t inode);
int file_read(char* buf, inode_t i);
void chdir(char* path);
void fs_mkdir(char* name);
void create_file(char* name);

void fs_stats();

#endif /* __FS_H */
