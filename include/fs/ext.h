#ifndef __EXT_H
#define __EXT_H

typedef enum __ext_flags {
    FS_FLAG_READ = 1 << 0,
    FS_FLAG_WRITE = 1 << 1,
    FS_FLAG_EXECUTE = 1 << 2,   
    FS_FLAG_CREATE = 1 << 3
} ext_flag_t;

#define FS_START_OFFSET
#include <fs/inode.h>

int init_ext();
void ext_sync();
void ext_create_file_system();

inode_t ext_open(char* name, ext_flag_t flags);
inode_t ext_open_from_directory(char* name, inode_t i);
void ext_close(inode_t inode);
int ext_read(inode_t i, void* buf, int size);
int ext_write(inode_t i, void* buf, int size);
inode_t change_directory(char* path);
inode_t ext_create_directory(char* name, inode_t);
int ext_create(char* name);

int ext_seek(inode_t i, int pos, int opt);

int ext_size(inode_t i);

inode_t ext_get_current_dir();
inode_t ext_get_root();

void ext_stats();

#endif /* __EXT_H */
