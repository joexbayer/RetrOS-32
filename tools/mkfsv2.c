#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <sync.h>

#include <fs/fat16.h>

#include "../tests/include/mocks.h"

FILE* filesystem = NULL;

int mkfsv2_load_bootloader()
{
    char bootblock[512];
    FILE* bootloader = fopen("bin/bootblock", "r");
    if(bootloader == NULL){
        return -1;
    }

    int size = fread(bootblock, 1, 512, bootloader);
    if(size != 512){
        return -2;
    }

    write_block(0, bootblock);

    fclose(bootloader);
    return 0;
}

int mkfsv2_load_kernel()
{
    int kernel_size;
    int kernel_block_count;

    FILE* kernel = fopen("bin/kernel", "r");
    if(kernel == NULL){
        return -1;
    }

    fseek(kernel, 0, SEEK_END);
    kernel_size = ftell(kernel);
    kernel_block_count = kernel_size / 512;
    fseek(kernel, 0, SEEK_SET);

    char* kernel_data = malloc(kernel_size);
    if(kernel_data == NULL){
        return -2;
    }

    int size = fread(kernel_data, 1, kernel_size, kernel);
    if(size != kernel_size){
        return -3;
    }

    for(int i = 0; i < kernel_block_count; i++){
        write_block(i+1, kernel_data + (i*512));
    }

    fclose(kernel);
    free(kernel_data);

    return kernel_block_count;
}

int main(int argc, char const *argv[])
{
    int kernel_block_count;
    int ret;

    filesystem = fopen("filesystemv2.img", "w+");
    if(filesystem == NULL){
        printf("Unable to open filesystem.");
        return -1;
    }

    ret = mkfsv2_load_bootloader();
    if(ret < 0){
        printf("Unable to load bootloader.\n");
        return -1;
    }

    kernel_block_count = mkfsv2_load_kernel();
    if(kernel_block_count <= 0){
        printf("Unable to load kernel.\n");
        return -1;
    }

    fat16_format("VOLUME1", kernel_block_count);
    if(fat16_initialize() < 0){
        printf("Unable to initialize FAT16 filesystem.\n");
        return -1;
    }

    return 0;
}