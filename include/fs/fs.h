#ifndef __FS_H
#define __FS_H

#define FS_START_OFFSET

#include <fs/inode.h>

int init_fs();
void mkfs();

inode_t open(char* name);
void file_close(inode_t inode);
void file_read(inode_t i);
void chdir(char* path);
void mkdir(char* name);
void create_file(char* name);

#endif /* __FS_H */
