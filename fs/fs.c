#include <fs/fs.h>

/* prototype functions for struct filesystem_ops */
static int default_write(struct filesystem* fs, struct file file, const void* buf, int size);
static int default_read(struct filesystem* fs, struct file file, void* buf, int size);
static int default_open(struct filesystem* fs, const char* path, int flags);
static int default_close(struct filesystem* fs, struct file file);
static int default_remove(struct filesystem* fs, const char* path);
static int default_mkdir(struct filesystem* fs, const char* path);
static int default_rmdir(struct filesystem* fs, const char* path);
static int default_rename(struct filesystem* fs, const char* path, const char* new_path);
static int default_stat(struct filesystem* fs, const char* path, struct file* file);
static int default_list(struct filesystem* fs, const char* path, char* buf, int size);

/* default filesystem_ops struct */
static struct filesystem_ops default_fs_ops = {
    .write = default_write,
    .read = default_read,
    .open = default_open,
    .close = default_close,
    .remove = default_remove,
    .mkdir = default_mkdir,
    .rmdir = default_rmdir,
    .rename = default_rename,
    .stat = default_stat,
    .list = default_list
};

/* default filesystem struct */
static struct filesystem default_fs = {
    .ops = &default_fs_ops,
    .flags = 0,
    .name = "default",
    .version = 0
};