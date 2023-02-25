#ifndef __PATH_H
#define __PATH_H

#include <stdint.h>
#include <fs/inode.h>

inode_t inode_from_path(char* path);

#endif /* __PATH_H */
