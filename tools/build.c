#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char* argv[])
{
    printf("[BUILD] Starting building...\n[BUILD] Creating NETOS iso file.\n");

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

    FILE* image = fopen("./boot.iso", "w+"); 

    printf("[BUILD] Bootblock: %d bytes\n", sz);
    printf("[BUILD] Kernel: %d bytes\n", sz_2);
    printf("[BUILD] Filesystem: %d bytes\n", sz_3);

    fseek(image, 0, SEEK_SET);
    
    #define SIZE (512)

    printf("[BUILD] Attaching bootblock to the iso file... ");
    char buffer[SIZE];
    size_t bytes;
    while (0 < (bytes = fread(buffer, 1, 512, bootblock))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, 512, image);
    }
    printf(" (DONE)\n");
    
    printf("[BUILD] Attaching kernel to the iso file... ");
    fseek(image, 512, SEEK_SET);
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), kernel))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }
    printf(" (DONE)\n");

    printf("[BUILD] Attaching filesystem to the iso file... ");
    int fs_start = ((sz_2/512)+2)*512;
    fseek(image, fs_start, SEEK_SET);
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), fs))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }
    printf(" (DONE)\n");

    fflush(image);

    
    fseek(image, 0L, SEEK_END);
    int sz_4 = ftell(image);
    rewind(image);
    printf("[BUILD] Image: %d bytes\n", sz_4);

    fclose(bootblock);
    fclose(kernel);
    fclose(fs);
    fclose(image);
    

    return 0;
}