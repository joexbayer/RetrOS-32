/**
 * @file mkfs.c
 * @author Joe Bayer (joexbayer)
 * @brief "Normal" C program, creating a RetrOS-32 filesystem and adds programs / files.
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
#include <sync.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>


#define FS_SIZE 1000000

#include <fs/ext.h>
#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

static FILE* filesystem = NULL;
static struct inode* root_dir;

#define FS_START_LOCATION 0
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

int get_current_time(struct time* time_s){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    time_s->year = tm.tm_year;
    time_s->month = tm.tm_mon+1;
    time_s->day = tm.tm_mday;
    time_s->hour = tm.tm_hour;
    time_s->minute = tm.tm_min;
    time_s->second = tm.tm_sec;

    return 0;
}

/* Functions needed for inode and bitmap to work. */
int32_t dbgprintf(char* fmt, ...)
{
    va_list argptr;
            va_start(argptr, fmt);
            vfprintf(stderr, fmt, argptr);
            va_end(argptr);
    return 0;
}

uint32_t serial_printf(char* fmt, ...)
{
    return 1;
}

void* kalloc(int size){
    return malloc(size);
}
void kfree(void* ptr)
{
    free(ptr);
}

void mutex_init(mutex_t* l)
{

}

void acquire(mutex_t* l)
{

}

void release(mutex_t* l)
{

}


/* Functions simulating the disk device read / write functions. */
int read_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    int ret = fread(buf, 1, 512, filesystem);
    if(ret <= 0)
        return 0;
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
    printf("[" BLUE "MKFS" RESET "] Added directory entry %s to inode %d\n", entry->name, inode->inode);
}


void ext_setup_superblock(struct superblock* superblock, int size)
{
    superblock->magic = MAGIC;
    superblock->size = size;

    superblock->ninodes = (superblock->size / (sizeof(struct inode)+NDIRECT*BLOCK_SIZE));
    superblock->nblocks = superblock->ninodes*NDIRECT;

    /* This will be recaculated at runtime in the kernel based on the kernel size. */
    superblock->inodes_start = FS_START_LOCATION + 3;
    superblock->blocks_start = superblock->inodes_start + (superblock->ninodes/ INODES_PER_BLOCK);

    superblock->block_map = create_bitmap(superblock->nblocks);
    superblock->inode_map = create_bitmap(superblock->ninodes);
}

int add_file(struct superblock* sb, struct inode* current_dir, char* program)
{
    /*char cwd[127];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("[" RED "MKFS" RESET "] Current working dir: %s\n", cwd);
    } else {
        printf("[" RED "MKFS" RESET "] Error\n");
    }*/

    FILE* file = fopen(program, "r");
    if(file == NULL){
        printf("[" RED "MKFS" RESET "] File %s not found!\n", program);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    int ext_size = ftell(file);
    rewind(file);

    char* buf = malloc(ext_size);
    int fret = fread(buf, 1, ext_size, file);
    if(fret <= 0){
        printf("[" RED "MKFS" RESET "] Error reading program %s!\n", program);
    }

    /* Skip to the first last */
    int name_offset = strlen(program);
    while(program[name_offset] != '/')
        name_offset--;
    name_offset++;

     /* Create a inode and write the contents of the given program.*/
    inode_t file_inode = alloc_inode(sb, FS_TYPE_FILE);
    //inode_t file_inode = alloc_inode(sb, FS_TYPE_FILE);
    printf(" INODE: %d\n", current_dir->inode);
    struct inode* file_inode_disk = inode_get(file_inode, sb);
    inode_write(buf, ext_size, file_inode_disk, sb);


    /* Add file to current dir */
    struct directory_entry file_dir_entry = {
        .inode = file_inode,
		.flags = FS_DIR_FLAG_FILE
    };
    memcpy(file_dir_entry.name, &program[name_offset], strlen(&program[name_offset])+1);

    printf(" INODE: %d\n", current_dir->inode);
    __inode_add_dir(&file_dir_entry, current_dir, sb);

    printf("[" BLUE "MKFS" RESET "] Added file %s (%d bytes)!\n", program, ext_size);

    free(buf);

    return 1;
}

int add_userspace_program(struct superblock* sb, struct inode* current_dir, char* program)
{   
    /* Open the file and copy content to buffer*/
    char path_buf[strlen("apps/")+strlen(program)+1];
    sprintf(path_buf, "%s%s", "apps/", program);
    printf("[" BLUE "MKFS" RESET "] Attaching %s (%s) to the filesystem!\n", program, path_buf);

    FILE* file = fopen(path_buf, "r");
    if(file == NULL){
        printf("[" RED "MKFS" RESET "] File %s not found!\n", program);
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    int ext_size = ftell(file);
    rewind(file);

    char* buf = malloc(ext_size+1);
    int fret = fread(buf, 1, ext_size, file);
    if(fret <= 0){
        printf("[" BLUE "MKFS" RESET "] Error reading program %s!\n", program);
    }

    /* Create a inode and write the contents of the given program.*/
    inode_t file_inode = alloc_inode(sb, FS_TYPE_FILE);
    struct inode* file_inode_disk = inode_get(file_inode, sb);
    inode_write(buf, ext_size, file_inode_disk, sb);
    

    /* Add file to current dir */
    struct directory_entry file_dir_entry = {
        .inode = file_inode,
		.flags = FS_DIR_FLAG_FILE
    };
    __inode_add_dir(&file_dir_entry, current_dir, sb);

    printf("[" BLUE "MKFS" RESET "] Added userspace program %s (%d bytes)!\n", program, ext_size);

    free(buf);

    return 1;   
}

int add_directory(struct superblock* sb, struct inode* parent, char* name)
{
    /* Create a root directory inode. */
    inode_t dir_inode = alloc_inode(sb, FS_TYPE_DIRECTORY);
    struct inode* dir_inode_disk = inode_get(dir_inode,sb);

    /* Basic directories */
    struct directory_entry self = {
        .inode = dir_inode,
        .name = ".",
		.flags = FS_DIR_FLAG_DIRECTORY
    };

    struct directory_entry back = {
        .inode = parent->inode,
        .name = "..",
		.flags = FS_DIR_FLAG_DIRECTORY
    };

    struct directory_entry in_parent = {
        .inode = dir_inode,
		.flags = FS_DIR_FLAG_DIRECTORY
    };
    memcpy(in_parent.name, name, strlen(name)+1);

    __inode_add_dir(&back, dir_inode_disk, sb);
    __inode_add_dir(&self, dir_inode_disk, sb);
    __inode_add_dir(&in_parent, parent, sb);

    printf("[" BLUE "MKFS" RESET "] Creating directory %s to the filesystem!\n", name);

    return dir_inode;
}

#define DT_DIR 4

void traverseDirectories(struct inode* root, struct superblock* superblock, const char *basePath, int depth) {
    DIR *dir;
    struct dirent *entry;
    inode_t iroot = root->inode;
    root->nlink++;

    // Open the directory
    dir = opendir(basePath);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    // Traverse the directory
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            
            for (int i = 0; i < depth; i++) printf("\t");

            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            printf("/%s\n", entry->d_name);
            int inode_index = add_directory(superblock, root, entry->d_name);
            struct inode* bin = inode_get(inode_index, superblock);

            // Recursively traverse subdirectories
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
            traverseDirectories(bin, superblock, path, depth+1);
        } else {

            for (int i = 0; i < depth; i++) printf("\t");
            

            printf("\t%s\n", entry->d_name);

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);

            root = inode_get(iroot, superblock);
            printf("[" BLUE "MKFS" RESET "] 2 Adding file %s to inode %d\n", path, root->inode);
            add_file(superblock, root, path);
        }
    }

    root->nlink--;


    // Close the directory
    closedir(dir);
}

int main(int argc, char* argv[])
{
    /* Make a filesystem image with given binary programs  */
    filesystem = fopen("filesystem.image", "w+");
    
    struct superblock superblock;
    ext_setup_superblock(&superblock, 1000000);

    /* Create a root directory inode. */
    inode_t root_inode = alloc_inode(&superblock, FS_TYPE_DIRECTORY);
    struct inode* root = inode_get(root_inode, &superblock);
    superblock.root_inode = root_inode;

    /* Basic directories */
    struct directory_entry self = {
        .inode = root_inode,
        .name = ".",
		.flags = FS_DIR_FLAG_DIRECTORY
    };

    struct directory_entry back = {
        .inode = root_inode,
        .name = "..",
		.flags = FS_DIR_FLAG_DIRECTORY
    };

    root_dir = root;

    __inode_add_dir(&back, root_dir, &superblock);
    __inode_add_dir(&self, root_dir, &superblock);

    /* Done */

    printf("[" BLUE "MKFS" RESET "] Creating Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
    printf("[" BLUE "MKFS" RESET "] With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
    printf("[" BLUE "MKFS" RESET "] And total of %d block\n", superblock.nblocks);
    printf("[" BLUE "MKFS" RESET "] Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);
    printf("[" BLUE "MKFS" RESET "] Written and saved filesystem to filesystem.image!\n");
    /* Save filesystem to disk! */
    const char *basePath = "./rootfs"; // Current directory
    traverseDirectories(root, &superblock, basePath, 0);

    //add_file(&superblock, root, "./rootfs/add.c");

    inodes_sync(&superblock);

    write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
    write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
    write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);


    /* Padding 0s */
    fseek(filesystem, 0L, SEEK_END);
    int sz = ftell(filesystem);
    printf("[" BLUE "MKFS" RESET "] Padding with %d bytes!\n", 1000000-sz);

    int left = 1000000-sz;
    while(left > 0){
        putc(0, filesystem);
        left--;
    }
    fclose(filesystem);

    return 0;
}