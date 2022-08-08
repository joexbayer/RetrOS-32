#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(int argc, char* argv[])
{
    FILE* bootblock = fopen("./bin/bootblock", "rw"); 
    fseek(bootblock, 0L, SEEK_END);
    int sz = ftell(bootblock);
    rewind(bootblock);

    FILE* kernel = fopen("./bin/kernelout", "rw"); 
    fseek(kernel, 0L, SEEK_END);
    int sz_2 = ftell(kernel);
    rewind(kernel);

    printf("Bootblock: %d\n", sz);
    printf("Kernel: %d\n", sz_2);

    return 0;
}