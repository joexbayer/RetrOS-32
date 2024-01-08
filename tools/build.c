#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sync.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <sync.h>

#include <fs/fat16.h>

#include "../tests/include/mocks.h"

#define BLOCK_SIZE 512
#define BOOTBLOCK_SIZE BLOCK_SIZE*4

FILE* filesystem = NULL;
#define DEBUG 1

/* TODO: as argv */
static const int IMG_SIZE = 32*1024*1024;

int build_load_bootloader()
{
    char bootblock[BOOTBLOCK_SIZE];
    FILE* bootloader = fopen("bin/bootblock", "r");
    if(bootloader == NULL){
        return -1;
    }

    int size = fread(bootblock, 1, BOOTBLOCK_SIZE, bootloader);
    if(size != BOOTBLOCK_SIZE){
        return -2;
    }

    /* write 4 bootblock blocks */
    for(int i = 0; i < 4; i++){
        write_block(bootblock + (i*512), i);
    }

    fclose(bootloader);
    return 0;
}

int build_load_kernel()
{
    int kernel_size;
    int kernel_block_count = 0;
    
    FILE* kernel = fopen("bin/kernelout", "r");
    if(kernel == NULL){
        return -1;
    }

    fseek(kernel, 0, SEEK_END);
    kernel_size = ftell(kernel);
    kernel_block_count = kernel_size / 512;
    fseek(kernel, 0, SEEK_SET);
    printf("Kernel size: %d\n", kernel_size);

    char* kernel_data = malloc(kernel_size);
    if(kernel_data == NULL){
        return -2;
    }

    int size = fread(kernel_data, 1, kernel_size, kernel);
    if(size != kernel_size){
        return -3;
    }

    for(int i = 0; i < kernel_block_count; i++){
        write_block(kernel_data + (i*512), i+4);
    }

    fclose(kernel);
    free(kernel_data);

    return kernel_block_count;
}

#include <stdio.h>
#include <string.h>

/**
 * @brief build - Create RetrOS images
 *
 * Usage: ./build [-opts] <kernel> 
 * Options
 *  -k Specify the kernel (Default: kernelnout)
 *  -b Specify bootloader (Default: bootblock)
 *  -d Disk only (no kernel or bootloader)
 *  -r Build release version
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return int Exit status
 */
int main(int argc, char const *argv[]) {

    char* outname;
    int disk_only = 0;
    int release = 0;
    
    int kernel_block_count = 0;
    int ret;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            i++;
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            i++;
        } else if (strcmp(argv[i], "-d") == 0) {
            disk_only = 1;
        } else if (strcmp(argv[i], "-r") == 0) {
            release = 1;
        } 
    }

    if(release){
        outname = "RetrOS-32.img";
    } else {
        outname = "RetrOS-32-debug.img";
    }

    if(disk_only){
        outname = "RetrOS-32-disk.img";
        printf("Building disk image only.\n");
    } else {
        printf("Building disk image with kernel and bootloader.\n");
    }

    filesystem = fopen(outname, "w+");
    if(filesystem == NULL){
        printf("Unable to open filesystem.");
        return -1;
    }

    if(!disk_only){
        ret = build_load_bootloader();
        if(ret < 0){
            printf("Unable to load bootloader: %d\n", ret);
            return -1;
        }

        kernel_block_count = build_load_kernel();
        if(kernel_block_count <= 0){
            printf("Unable to load kernel: %d\n", kernel_block_count);
            return -1;
        }
    }

    /* We want to reserve blocks for the kernel before the filesystem starts. */

    fat16_format("VOLUME1", kernel_block_count+4);
    if(fat16_load() < 0){
        printf("Unable to initialize FAT16 filesystem.\n");
        return -1;
    }


    fseek(filesystem, 0, SEEK_END);
    int size2 = ftell(filesystem);
    printf("Size of filesystem: %d\n", size2);
    /* pad to 32mb */
    if(size2 < IMG_SIZE){
        char* buf = malloc(IMG_SIZE - size2);
        memset(buf, 0, IMG_SIZE - size2);
        fwrite(buf, IMG_SIZE - size2, 1, filesystem);
        free(buf);

        printf("Padded filesystem to 32mb.\n");
    }   

    return 0;
}