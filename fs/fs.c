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

    file->nlinks = 0;
    file->offset = 0;
    file->flags = 0;
    file->identifier = 0;
    file->directory = 0;
    
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

struct filesystem* fs_get()
{
    return fs_current;
}