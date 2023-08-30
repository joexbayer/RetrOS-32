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

int load_bootloader()
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

int main(int argc, char const *argv[])
{
    filesystem = fopen("filesystemv2.img", "w+");
    if(filesystem == NULL){
        printf("Unable to open filesystem.");
        return -1;
    }

    fat16_format("VOLUME1", 0);
    if(fat16_initialize() < 0){
        printf("Unable to initialize FAT16 filesystem.\n");
        return -1;
    }

    return 0;
}