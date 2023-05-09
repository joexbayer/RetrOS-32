#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

int main(int argc, char* argv[])
{
    printf("[" BLUE "BUILD" RESET "] Starting building...\n[" BLUE "BUILD" RESET "] Creating NETOS iso file.\n");

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

    printf("[" BLUE "BUILD" RESET "] Bootblock: %d bytes\n", sz);
    printf("[" BLUE "BUILD" RESET "] Kernel: %d bytes\n", sz_2);
    printf("[" BLUE "BUILD" RESET "] Filesystem: %d bytes\n", sz_3);

    fseek(image, 0, SEEK_SET);
    
    #define SIZE (512)

    printf("[" BLUE "BUILD" RESET "] Attaching bootblock to the iso file... ");
    char buffer[SIZE];
    size_t bytes;
    while (0 < (bytes = fread(buffer, 1, 512, bootblock))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, 512, image);
    }
    printf(" (DONE)\n");
    
    printf("[" BLUE "BUILD" RESET "] Attaching kernel to the iso file... ");
    fseek(image, 512, SEEK_SET);
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), kernel))){
        //printf("Writing %d bytes to image\n", bytes);
        fwrite(buffer, 1, bytes, image);
    }
    printf(" (DONE)\n");

    printf("[" BLUE "BUILD" RESET "] Attaching filesystem to the iso file... ");
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
    printf("[" BLUE "BUILD" RESET "] Image: %d bytes\n", sz_4);

    fclose(bootblock);
    fclose(kernel);
    fclose(fs);
    fclose(image);
    

    return 0;
}