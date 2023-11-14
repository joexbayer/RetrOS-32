#ifndef __FS_MODULE_H
#define __FS_MODULE_H

#include <util.h>

#define FS_VERSION 1
#define FS_VALIDATE(fs) if(!fs || fs->version != FS_VERSION) return -1;

struct filesystem;
struct file;

/* file flags enum */
typedef enum __fs_file_flags {
    FS_FILE_FLAG_READ = 1 << 0,
    FS_FILE_FLAG_WRITE = 1 << 1,
    FS_FILE_FLAG_EXECUTE = 1 << 2,
    FS_FILE_FLAG_CREATE = 1 << 3
} fs_file_flag_t;

struct file {
    /* flags: fs_file_flag_t*/
    char flags;
    int offset;
    /* amount of owners */
    int nlinks;
    int identifier;
    int directory;
};

/* none of the functions can ever be NULL */
struct filesystem_ops {
    /* basic functionality */
    int (*write)(struct filesystem* fs, struct file* file, const void* buf, int size);
    int (*read)(struct filesystem* fs, struct file* file, void* buf, int size);
    struct file* (*open)(struct filesystem* fs, const char* path, int flags);
    int (*close)(struct filesystem* fs, struct file* file);

    /* extended functionality */
    int (*remove)(struct filesystem* fs, const char* path);
    int (*mkdir)(struct filesystem* fs, const char* path);
    int (*rmdir)(struct filesystem* fs, const char* path);
    int (*rename)(struct filesystem* fs, const char* path, const char* new_path);
    int (*stat)(struct filesystem* fs, const char* path, struct file* file);
    int (*list)(struct filesystem* fs, const char* path, char* buf, int size);
};

/* filesystem flags as enum */
typedef enum __fs_flags {
    FS_FLAG_INITILIZED = 1 << 0,
    FS_FLAG_UNUSED = 1 << 1,
} fs_flag_t;

typedef enum __fs_seek_flags {
    FS_SEEK_START = 0,
    FS_SEEK_CUR = 1
} fs_seek_flag_t;

struct filesystem {
    struct filesystem_ops* ops;
    unsigned char flags;
    char name[32];
    int version;
    int type;
};

/* filesystem functions */
int fs_register(struct filesystem* fs);
int fs_unregister(struct filesystem* fs);

int fs_load_from_file(const char* file, void* buf, int size);

struct file* fs_alloc_file();

int fs_init();

int fs_seek(int fd, int offset, fs_seek_flag_t flag);

int fs_open(const char* path, int flags);
int fs_close(int fd);
int fs_read(int fd, void* buf, int size);
int fs_write(int fd, void* buf, int size);


#endif /* !__FS_MODULE_H */
