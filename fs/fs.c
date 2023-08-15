#include <fs/fs.h>
#include <assert.h>
#include <serial.h>
#include <errors.h>

/* This determines the maximum of simultaneously open files */
#define FS_MAX_FILES 256

/* filesystem macros */

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

/* filesystem table */
static struct file fs_file_table[FS_MAX_FILES] = {0};

/* default filesystem functions */
static int default_write(struct filesystem* fs, struct file file, const void* buf, int size)
{
    ERR_ON_NULL(fs);

    return -1;
}

static int default_read(struct filesystem* fs, struct file file, void* buf, int size)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_open(struct filesystem* fs, const char* path, int flags)
{
    ERR_ON_NULL(fs);
    struct file* file = NULL;

    /* find a free file */
    for(int i = 0; i < FS_MAX_FILES; i++){
        if(fs_file_table[i].nlinks == 0){
            file = &fs_file_table[i];
            break;
        }
    }

    if(!file){
        return -1;
    }

    file->nlinks++;
    file->flags = flags;
    file->offset = 0;


    return -1;
}

static int default_close(struct filesystem* fs, struct file file)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_remove(struct filesystem* fs, const char* path)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_mkdir(struct filesystem* fs, const char* path)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_rmdir(struct filesystem* fs, const char* path)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_rename(struct filesystem* fs, const char* path, const char* new_path)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_stat(struct filesystem* fs, const char* path, struct file* file)
{
    ERR_ON_NULL(fs);
    return -1;
}

static int default_list(struct filesystem* fs, const char* path, char* buf, int size)
{
    ERR_ON_NULL(fs);
    return -1;
}

/* filesystem functions */
int fs_init()
{
    return 0;
}

int fs_register(struct filesystem* fs)
{
    return 0;
}

int fs_unregister(struct filesystem* fs)
{
    return 0;
}

struct filesystem* fs_get(const char* name)
{
    return &default_fs;
}