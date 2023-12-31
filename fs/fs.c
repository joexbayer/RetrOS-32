/**
 * @file fs.c
 * @author Joe Bayer (joexbayer)
 * @brief In memory Filesystem abstraction implementation
 * @version 0.1
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 * The abstraction layer which is used by the kernel to access the filesystem.
 * Hides which filesystem is used, and provides a common interface for the kernel.
 * Controls the open files and their offsets, this also a "file" in the filesystem can have multiple
 * open in memory files. As a file only cares about the offset and the flags.
 * 
 */

#include <fs/fs.h>
#include <assert.h>
#include <serial.h>
#include <errors.h>

/* This determines the maximum of simultaneously open files */
#define FS_MAX_FILES 256
#define FS_MAX_FILESYSTEMS 4 /* maximum number of partitons entries in MBR */

/* default filesystem struct */
static struct file fs_file_table[FS_MAX_FILES];
static struct filesystem* fs_table[FS_MAX_FILESYSTEMS] = {NULL};
static struct filesystem* fs_current = NULL;

int fs_file2fd(struct file* file)
{
    ERR_ON_NULL(file);

    for (int i = 0; i < FS_MAX_FILES; i++){
        if(&fs_file_table[i] == file){
            return i;
        }
    }

    return -1;
}

/* functions used by filesystem implementations */
struct file* fs_alloc_file()
{
    struct file* file = NULL;

    /* find a free file */
    for(int i = 0; i < FS_MAX_FILES; i++){
        if(fs_file_table[i].flags == 0){
            file = &fs_file_table[i];
            break;
        }
    }

    /* no free file found */
    if(file == NULL){
        return NULL;
    }

    memset(file, 0, sizeof(struct file));
    return file;
}

int fs_free_file(struct file* file)
{
    ERR_ON_NULL(file);

    file->nlinks = 0;
    file->offset = 0;
    file->flags = 0;
    file->identifier = 0;
    file->directory = 0;

    return 0;
}

int fs_register(struct filesystem* fs)
{
    ERR_ON_NULL(fs);

    /* check if the filesystem has ALL ops the required functions */
    if(fs->ops->open == NULL || fs->ops->read == NULL || fs->ops->write == NULL || fs->ops->close == NULL){
        return -1;
    }

    /* find a free filesystem slot */
    for(int i = 0; i < FS_MAX_FILESYSTEMS; i++){
        if(fs_table[i] == NULL){
            fs_table[i] = fs;

            if(fs_current == NULL) fs_current = fs;
            dbgprintf("Registered filesystem: %s\n", fs->name);
            return 0;
        }
    }

    /* no free filesystem slot found */
    return -1;
}

/* filesystem functions */
int fs_init()
{
    
    return 0;
}

int fs_unregister(struct filesystem* fs)
{
    ERR_ON_NULL(fs);

    /* find the filesystem */
    for(int i = 0; i < FS_MAX_FILESYSTEMS; i++){
        if(fs_table[i] == fs){
            fs_table[i] = NULL;
            return 0;
        }
    }

    /* filesystem not found */
    return -1;
}

/**
 * @brief Returns the current filesystem.
 * 
 * @return struct filesystem* (NULL if no filesystem is available)
 */
struct filesystem* fs_get()
{
    return fs_current;
}

int fs_load_from_file(const char* file, void* buf, int size)
{
	int inode = fs_open(file, FS_FILE_FLAG_READ);
	if(inode < 0){
		dbgprintf("Error opening %s\n", file);
		return -ERROR_FILE_NOT_FOUND;
	}

	dbgprintf("Reading %s from disk\n", file);
	int read = fs_read(inode, buf, size);
	if(read < 0){
		fs_close(inode);
		return -ERROR_FILE_NOT_FOUND;
	}

	fs_close(inode);
	return read;
}

int fs_close(int fd)
{
    /* check if a filesystem is available */
    if(fs_current == NULL){
        return -1;
    }

    /* check if the file is open */
    if(fd < 0 || fd >= FS_MAX_FILES || fs_file_table[fd].flags == 0){
        return -2;
    }

    /* close the file */
    int ret = fs_current->ops->close(fs_current, &fs_file_table[fd]);
    if(ret < 0){
        return -3;
    }

    fs_free_file(&fs_file_table[fd]);

    return 0;
}

/* function that use the default filesystem and a "file descriptor" system, if avaiable */
int fs_open(const char* path, int flags)
{
    ERR_ON_NULL(path);

    /* check if a filesystem is available */
    if(fs_current == NULL || fs_current->ops->open == NULL){
        return -1;
    }

    /* open the file */
    struct file* file = fs_current->ops->open(fs_current, path, flags);
    if(file == NULL){
        return -1;
    }

    int fd = fs_file2fd(file);

    return fd;
}

int fs_seek(int fd, int offset, fs_seek_flag_t flag)
{
    /* check if a filesystem is available */
    if(fs_current == NULL){
        return -1;
    }

    /* check if the file is open */
    if(fd < 0 || fd >= FS_MAX_FILES || fs_file_table[fd].flags == 0){
        return -2;
    }

    switch (flag){
    case FS_SEEK_START:
        fs_file_table[fd].offset = offset;
        break;
    
    case FS_SEEK_CUR:
        fs_file_table[fd].offset += offset;
        break;
    default:
        break;
    }

    return 0;
}

int fs_read(int fd, void* buf, int size)
{
    ERR_ON_NULL(buf);

    /* check if a filesystem is available */
    if(fs_current == NULL || fs_current->ops->read == NULL){
        dbgprintf("No filesystem available %x %x\n", fs_current, fs_current->ops->read);
        return -1;
    }

    /* check if the file is open */
    if(fd < 0 || fd >= FS_MAX_FILES || !HAS_FLAG(fs_file_table[fd].flags, FS_FILE_FLAG_READ)){
        dbgprintf("File not open %d %d %d\n", fd, fs_file_table[fd].flags, FS_FILE_FLAG_READ);
        return -2;
    }

    /* read the file */
    int ret = fs_current->ops->read(fs_current, &fs_file_table[fd], buf, size);
    if(ret <= 0){
        return -3;
    }

    fs_file_table[fd].offset += size;

    dbgprintf("fs_read %d: read %d/%d\n", fd, ret, size);

    return ret;
}

int fs_write(int fd, void* buf, int size)
{
    ERR_ON_NULL(buf);

    /* check if a filesystem is available */
    if(fs_current == NULL || fs_current->ops->write == NULL){
        dbgprintf("No filesystem available\n");
        return -1;
    }

    /* check if the file is open */
    if(fd < 0 || fd >= FS_MAX_FILES || !(fs_file_table[fd].flags & FS_FILE_FLAG_WRITE)){
        dbgprintf("File not open\n");
        return -2;
    }

    /* write the file */
    int ret = fs_current->ops->write(fs_current, &fs_file_table[fd], buf, size);
    if(ret < 0){
        return -3;
    }

    return ret;
}