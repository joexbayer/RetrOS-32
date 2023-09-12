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
#include <memory.h>

#include <terminal.h>

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

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
    struct filesystem* fs = (struct filesystem*)kalloc(sizeof(struct filesystem));
    ERR_ON_NULL(fs);

    fs->ops = &fat16_ops;
    fs->flags = FS_FLAG_INITILIZED;
    fs->version = FS_VERSION;
    fs->type = MBR_TYPE_FAT16_LBA;

    memcpy(fs->name, "fat16", 6);

    fs_register(fs);

    fat16_list(fs, "/", NULL, 0);

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
        dbgprintf("File is not open\n");
        return -1;
    }

    /* check if the file is writeable */
    if(!(file->flags & FS_FILE_FLAG_WRITE)){
        dbgprintf("File is not writeable\n");
        return -2;
    }

    struct fat16_directory_entry entry;
    if(fat16_read_entry(file->directory, file->identifier, &entry) != 0){
        dbgprintf("Failed to read directory entry\n");
        return -3;
    }

    int cluster = entry.first_cluster;
    int offset = /*file->offset*/ 0;

    /* write the data */
    int written = fat16_write_data(cluster, offset, (void*)buf, size);
    if(written < 0){
        dbgprintf("Failed to write data\n");
        return -4;
    }

    /* update the file size */
    entry.file_size += size;

    fat16_sync_directory_entry(file->directory, file->identifier, &entry);

    dbgprintf("Wrote %d bytes to cluster %d offset %d\n", written, cluster, offset);

    return size;
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

    dbgprintf("Entry: \n");
    dbgprintf("  Filename: %s\n", entry.filename);
    dbgprintf("  Attributes: 0x%x\n", entry.attributes);
    dbgprintf("  First cluster: %d\n", entry.first_cluster);
    dbgprintf("  File size: %d\n", entry.file_size);


    int cluster = entry.first_cluster;
    int offset = file->offset;
    int file_size = IS_DIRECTORY(entry) ? 512 : entry.file_size;

    /* read the data */
    dbgprintf("Reading %d bytes from cluster %d offset %d\n", size, cluster, offset);

    int read = fat16_read_data(cluster, offset, buf, size, file_size);
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
    struct file* file;
    struct fat16_file_identifier id;;
    struct fat16_directory_entry entry;

    if(fs == NULL || path == NULL || strlen(path) > 255){
        return NULL;
    }

    /* parse path */
    id = fat16_get_directory_entry((char*)path, &entry);
    if(id.directory < 0){
        dbgprintf("Failed to get directory entry\n");
        return NULL;
    }

    /* allocate new file */
    file = fs_alloc_file();
    if(file == NULL){
        return NULL;
    }

    file->flags = flags;
    file->offset = 0;
    file->directory = id.directory;
    file->identifier = id.index; 
    file->nlinks = 1;

    dbgprintf("File %s:\n", path);
    dbgprintf("  Directory: %d\n", file->directory);
    dbgprintf("  Identifier: %d\n", file->identifier);
    dbgprintf("  Flags: 0x%x\n", file->flags);
    


    return file;
}

/**
 * @brief Closes a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param file The file to close.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_close(struct filesystem* fs, struct file* file)
{
    FS_VALIDATE(fs);
    ERR_ON_NULL(file);

    /* check if the file is open */
    if(file->nlinks == 0){
        return -1;
    }

    /* close the file */
    file->nlinks = 0;

    return 0;
}

/**
 * @brief Removes a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the file.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_remove(struct filesystem* fs, const char* path)
{
    return 0;
}

/**
 * @brief Creates a directory.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the directory.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_mkdir(struct filesystem* fs, const char* path)
{
    return 0;
}

/**
 * @brief Removes a directory.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the directory.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_rmdir(struct filesystem* fs, const char* path)
{
    return 0;
}

/**
 * @brief Renames a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the file.
 * @param new_path The new path to the file.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_rename(struct filesystem* fs, const char* path, const char* new_path)
{
    return 0;
}

/**
 * @brief Gets the status of a file.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the file.
 * @param file The file to write the status to.
 * @return int 0 on success, or a negative value on error. 
 */
static int fat16_stat(struct filesystem* fs, const char* path, struct file* file)
{
    return 0;
}

/**
 * @brief Lists the contents of a directory.
 * 
 * @package fs
 * @param fs The filesystem to use.
 * @param path The path to the directory.
 * @param buf The buffer to write the contents to.
 * @param size The size of the buffer.
 * @return int The number of bytes written, or a negative value on error. 
 */
static int fat16_list(struct filesystem* fs, const char* path, char* buf, int size)
{
    struct fat16_directory_entry entry;
    int entries = 0;
    struct fat16_file_identifier id = fat16_get_directory_entry((char*)path, &entry);
    if(id.directory < 0 || entry.attributes != FAT16_FLAG_SUBDIRECTORY){
        return -1;
    }
    
    dbgprintf("Listing directory %d\n", id.directory);

    /**
     * @brief Should this function use twrite?
     * This would assume there is a terminal attached to the system
     * and would not work if there is not. Other processes such as a finder
     * would need to use a different method to list the contents of a directory.
     */

    /* print the directory contents */
    twritef("Size  Date    Time    Name\n");
    for (int i = 0; i < (int)ENTRIES_PER_BLOCK; i++) {
        struct fat16_directory_entry entry = {0};
        struct fat16_directory_entry* dir_entry = &entry;

        fat16_read_entry(id.directory, i, &entry);

        /* Check if the entry is used (filename's first byte is not 0x00 or 0xE5) */
        if (dir_entry->filename[0] != 0x00 && dir_entry->filename[0] != 0xE5
            // && (HAS_FLAG(dir_entry->attributes, FAT16_FLAG_ARCHIVE) || HAS_FLAG(dir_entry->attributes, FAT16_FLAG_SUBDIRECTORY))
        ) {
            /* parse name */
            char name[13] = {0};
            int j = 0;
            while(dir_entry->filename[j] != ' '){
                name[j] = dir_entry->filename[j];
                j++;
            }

            if(dir_entry->extension[0] != ' '){
                name[j++] = '.';
                for(int k = 0; k < 3; k++){
                    name[j++] = dir_entry->extension[k];
                }
            }

            name[j] = '\0';

            /* get time */
            uint16_t time = dir_entry->created_time;
            uint8_t seconds = (time & 0x1F) * 2;
            uint8_t minutes = (time >> 5) & 0x3F;
            uint8_t hours = (time >> 11) & 0x1F;

            /* get date */
            uint16_t date = dir_entry->created_date;
            uint8_t day = date & 0x1F;
            uint8_t month = (date >> 5) & 0x0F;
            uint16_t year = 1980 + ((date >> 9) & 0x7F);

            twritef("%p %s %d, %d:%d - %s%s\n",
                dir_entry->file_size,
                months[month],
                day, hours, minutes,
                name,
                dir_entry->attributes & FAT16_FLAG_SUBDIRECTORY ? "/" : ""
            );

            entries++;
        }
    }

    twritef("%d directory entries.\n", entries);

    return 0;
}