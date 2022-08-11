/**
 * @file mkfs.c
 * @author Joe Bayer (joexbayer)
 * @brief "Normal" C program, creating a NETOS filesystem and adds programs / files.
 * Compiled to 32 bit as it is linked with the ENTOS fs/ objects and headers. 
 * @version 0.1
 * @date 2022-08-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>


#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>

static FILE* filesystem = NULL;
static struct inode* root_dir;

#define FS_START_LOCATION 0
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

int get_current_time(struct time* time_s){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    time_s->year = tm.tm_year;
    time_s->month = tm.tm_mon;
    time_s->day = tm.tm_mday;
    time_s->hour = tm.tm_hour;
    time_s->minute = tm.tm_min;
    time_s->second = tm.tm_sec;
}

/* Functions needed for inode and bitmap to work. */
int32_t dbgprintf(char* fmt, ...)
{
    return 0;
}

void* alloc(int size){
    return malloc(size);
}

/* Functions simulating the disk device read / write functions. */
int read_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    fread(buf, 1, 512, filesystem);
    return 1;
}

int write_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    fwrite(buf, 1, 512, filesystem);
    return 1;
}

int write_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block(buf, block);
    memcpy(&buf[offset], usr_buf, size);

    return write_block(buf, block);
}

int read_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block((char*)buf, block);
    memcpy(usr_buf, &buf[offset], size);

    return size;   
}

/* Helper function used by fs */
static inline void __inode_add_dir(struct directory_entry* entry, struct inode* inode, struct superblock* sb)
{
    inode->pos = inode->size;
    inode_write((char*) entry, sizeof(struct directory_entry), inode, sb);
}

int main(int argc, char* argv[])
{

    /* Make a filesystem image with given binary programs  */
    filesystem = fopen("filesystem.image", "w+");

    struct superblock superblock;

    superblock.magic = MAGIC;
    superblock.size = 200000; // TODO: turn into argv parameter.

    superblock.ninodes = (superblock.size / (sizeof(struct inode)+NDIRECT*BLOCK_SIZE));
    superblock.nblocks = superblock.ninodes*NDIRECT;

    /* This will be recaculated at runtime in the kernel based on the kernel size. */
    superblock.inodes_start = FS_START_LOCATION + 3;
    superblock.blocks_start = superblock.inodes_start + (superblock.ninodes/ INODES_PER_BLOCK);

    superblock.block_map = create_bitmap(superblock.nblocks);
    superblock.inode_map = create_bitmap(superblock.ninodes);

    inode_t root_inode = alloc_inode(&superblock, FS_DIRECTORY);
    struct inode* root = inode_get(root_inode, &superblock);
    superblock.root_inode = root_inode;

    /* Basic directories */
    struct directory_entry self = {
        .inode = root_inode,
        .name = "."
    };

    struct directory_entry back = {
        .inode = root_inode,
        .name = ".."
    };

    inode_t home_inode = alloc_inode(&superblock, FS_FILE);
    struct inode* home_disk_inode = inode_get(home_inode, &superblock);

    struct directory_entry home = {
        .inode = home_inode,
        .name = "home"
    };

    char* home_text = "Home is where the heart is.";
    inode_write(home_text, strlen(home_text)+1, home_disk_inode, &superblock);

    root_dir = root;

    __inode_add_dir(&back, root_dir, &superblock);
    __inode_add_dir(&self, root_dir, &superblock);
    __inode_add_dir(&home, root_dir, &superblock);

    inodes_sync(&superblock);

    write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
    write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);

    printf("[FS]: Creating Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
    printf("[FS]: With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
    printf("[FS]: And total of %d block\n", superblock.nblocks);
    printf("[FS]: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);
    printf("[FS] Written and saved filesystem to filesystem.image!\n");

    fclose(filesystem);

    return 0;
}