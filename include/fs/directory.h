#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <fs/inode.h>

#define FS_DIRECTORY_NAME_SIZE 20
typedef enum __fs_type {
    FS_TYPE_FILE = 1,
    FS_TYPE_DIRECTORY = 2
} fs_type_t;

/* directory flags as enum */
typedef enum __fs_directory_flags {
    FS_DIR_FLAG_UNUSED = 1 << 0,
    FS_DIR_FLAG_DIRECTORY = 1 << 1,
    FS_DIR_FLAG_FILE = 1 << 2
} fs_directory_flag_t;


struct directory_entry {
    char name[FS_DIRECTORY_NAME_SIZE];
    inode_t inode;
    char flags;
};

#endif