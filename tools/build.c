#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char* argv[])
{
    FILE* bootblock = fopen("./bin/bootblock", "rwb"); 
    fseek(bootblock, 0L, SEEK_END);
    int sz = ftell(bootblock);
    rewind(bootblock);

    FILE* kernel = fopen("./bin/kernelout", "rwb"); 
    fseek(kernel, 0L, SEEK_END);
    int sz_2 = ftell(kernel);
    rewind(kernel);

    FILE* fs = fopen("./filesystem.image", "rwb"); 
    fseek(fs, 0L, SEEK_END);
    int sz_3 = ftell(fs);
    rewind(fs);

    FILE* image = fopen("./boot2.iso", "w+"); 
    fseek(image, 0L, SEEK_END);
    int sz_4 = ftell(image);
    rewind(image);

    printf("Bootblock: %d\n", sz);
    printf("Kernel: %d\n", sz_2);
    printf("Filesystem: %d\n", sz_3);
    printf("Image: %d\n", sz_4);

    fseek(image, 0, SEEK_SET);
    
    #define SIZE (1024)

    char buffer[SIZE];
    size_t bytes;
    while (0 < (bytes = fread(buffer, 1, 512, bootblock))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }
    
    fseek(image, 512, SEEK_SET);
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), kernel))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }

    int fs_start = ((sz_2/512)+2)*512;
    printf("Fs_start: %d, %d\n", fs_start, fs_start/512);

    fseek(image, fs_start, SEEK_SET);
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), fs))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }

    fflush(image);

    fclose(bootblock);
    fclose(kernel);
    fclose(fs);
    fclose(image);
    

    return 0;
}