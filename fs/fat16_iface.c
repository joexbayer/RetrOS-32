/**
 * @file fat16_iface.c
 * @author Joe Bayer (joexbayer)
 * @brief Interface for the FAT16 filesystem to implements the filesystem_ops struct
 * @version 0.1
 * @date 2023-05-31
 *
 * @copyright Copyright (c) 2023
 * 
 */

#include <util.h>
#include <fs/fat16.h>
#include <fs/fs.h>
#include <kutils.h>
#include <errors.h>

/* filesystem_ops prototypes */
static int fat16_write(struct filesystem* fs, struct file* file, const void* buf, int size);
static int fat16_read(struct filesystem* fs, struct file* file, void* buf, int size);
static struct file* fat16_open(struct filesystem* fs, const char* path, int flags);
static int fat16_close(struct filesystem* fs, struct file* file);
static int fat16_remove(struct filesystem* fs, const char* path);
static int fat16_mkdir(struct filesystem* fs, const char* path);
static int fat16_rmdir(struct filesystem* fs, const char* path);
static int fat16_rename(struct filesystem* fs, const char* path, const char* new_path);
static int fat16_stat(struct filesystem* fs, const char* path, struct file* file);
static int fat16_list(struct filesystem* fs, const char* path, char* buf, int size);

/* filesystem_ops struct */
static struct filesystem_ops fat16_ops = {
    .write = fat16_write,
    .read = fat16_read,
    .open = fat16_open,
    .close = fat16_close,
    .remove = fat16_remove,
    .mkdir = fat16_mkdir,
    .rmdir = fat16_rmdir,
    .rename = fat16_rename,
    .stat = fat16_stat,
    .list = fat16_list
};


int fat16_init()
{
    /* load the fat16 filesystem */
    if(fat16_load() != 0){
        dbgprintf("Failed to load fat16 filesystem\n");
        return -1;
    }

    /* register the filesystem */
    struct filesystem* fs = (struct filesystem*)kmalloc(sizeof(struct filesystem));
    ERR_ON_NULL(fs);

    fs->ops = &fat16_ops;
    fs->flags = FS_FLAG_INITILIZED;
    fs->version = FS_VERSION;
    fs->type = MBR_TYPE_FAT16_LBA;

    memcpy(fs->name, "fat16", 6);

    fs_register(fs);

    return 0;
}

/**
 * @brief Writes data to a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param file The file to write to.
 * @param buf The buffer to write from.
 * @param size The size of the buffer.
 * @return int The number of bytes written, or a negative value on error. 
 */
static int fat16_write(struct filesystem* fs, struct file* file, const void* buf, int size)
{
    FS_VALIDATE(fs);

    /* check if the file is open */
    if(file->nlinks == 0){
        return -1;
    }

    /* check if the file is writeable */
    if(!(file->flags & FS_FILE_FLAG_WRITE)){
        return -2;
    }

    struct fat16_directory_entry entry;
    if(fat16_read_entry(file->directory, file->identifier, &entry) != 0){
        return -3;
    }

    int cluster = entry.first_cluster;
    int offset = file->offset;

    /* write the data */
    int written = fat16_write_data(cluster, offset, (void*)buf, size);
    if(written < 0){
        return -4;
    }

    /* update the file size */
    entry.file_size += written;

    fat16_sync_directory_entry(file->directory, file->identifier, &entry);

    return written;
}

/**
 * @brief Reads data from a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param file The file to read from.
 * @param buf The buffer to read into.
 * @param size The size of the buffer.
 * @return int The number of bytes read, or a negative value on error. 
 */
static int fat16_read(struct filesystem* fs, struct file* file, void* buf, int size)
{
    FS_VALIDATE(fs);
    ERR_ON_NULL(file);

    /* check if the file is open */
    if(file->nlinks == 0){
        return -1;
    }

    /* check if the file is readable */
    if(!(file->flags & FS_FILE_FLAG_READ)){
        return -2;
    }

    struct fat16_directory_entry entry;
    if(fat16_read_entry(file->directory, file->identifier, &entry) != 0){
        return -3;
    }

    int cluster = entry.first_cluster;
    int offset = file->offset;

    /* read the data */
    int read = fat16_read_data(cluster, offset, buf, size, entry.file_size);
    if(read < 0){
        return -4;
    }

    return read;
}

/**
 * @brief Opens a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the file.
 * @param flags The flags to open the file with.
 * @return struct file* The file descriptor, or NULL on error. 
 */
static struct file* fat16_open(struct filesystem* fs, const char* path, int flags)
{
    FS_VALIDATE(fs);

    /* check if the path is too long */
    if(strlen(path) > 255){
        return NULL;
    }

    /* parse path */

    /* check if the file is already open */
    struct file* file = fs_alloc_file();
    ERR_ON_NULL(file);


    return file;
}



