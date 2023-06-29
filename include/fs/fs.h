#ifndef __FS_MODULE_H
#define __FS_MODULE_H

#include <utils.h>

struct file {
    int flags;
    int offset;
    int size;
    /* TODO: inode for ext, ?? for fat16 */
};

/* none of the functions can ever be NULL */
struct filesystem_ops {
    int (*write)(struct filesystem* fs, struct file file, const void* buf, int size);
    int (*read)(struct filesystem* fs, struct file file, void* buf, int size);
    int (*open)(struct filesystem* fs, const char* path, int flags);
    int (*close)(struct filesystem* fs, struct file file);
    int (*remove)(struct filesystem* fs, const char* path);
    int (*mkdir)(struct filesystem* fs, const char* path);
    int (*rmdir)(struct filesystem* fs, const char* path);
    int (*rename)(struct filesystem* fs, const char* path, const char* new_path);
    int (*stat)(struct filesystem* fs, const char* path, struct file* file);
    int (*list)(struct filesystem* fs, const char* path, char* buf, int size);
};

struct filesystem {
    struct filesystem_ops* ops;
    unsigned char flags;
    char name[32];
    int version;
};

#endif /* !__FS_MODULE_H */
