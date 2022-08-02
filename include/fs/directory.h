#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <fs/inode.h>

#define FS_DIRECTORY_NAME_SIZE 20

struct directory_entry {
    char name[FS_DIRECTORY_NAME_SIZE];
    inode_t inode;
};

#endif