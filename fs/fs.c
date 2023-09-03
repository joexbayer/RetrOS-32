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

/* default filesystem struct */
static struct filesystem* current_fs = NULL;

/* functions used by filesystem implementations */
struct file* fs_alloc_file()
{
    struct file* file = (struct file*)kmalloc(sizeof(struct file));
    ERR_ON_NULL(file);

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

    kfree(file);

    return 0;
}

int fs_register(struct filesystem* fs)
{
    ERR_ON_NULL(fs);

    if(fs->ops == NULL){
        return -1;
    }

    /* check if the filesystem is already registered */
    if(current_fs != NULL){
        return -2;
    }

    /* set the current filesystem */
    current_fs = fs;

    return 0;
}

/* filesystem functions */
int fs_init()
{
    return 0;
}

int fs_unregister(struct filesystem* fs)
{
    return 0;
}

struct filesystem* fs_get()
{
    return current_fs;
}