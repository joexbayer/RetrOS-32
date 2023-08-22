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

#include <mocks.h>

#define DEBUG 1

/* needed by mocks.c */
FILE* filesystem = NULL;

int main(int argc, char const *argv[])
{

    filesystem = fopen("filesystem.test", "w+");
    if(filesystem == NULL){
        printf("Unable to open mock filesystem.");
        return -1;
    }

    fat16_format();


    fat16_create_file("sample", "txt", "Hello, FAT16!", strlen("Hello, FAT16!")+1);

    char buf1[512];
    int ret1 = fat16_read_file("sample", "txt", buf1, 14);
    if (ret1 <= 0) {
        printf("Unable to read file (sample.txt).\n");
        return -1;
    }
    printf("%s (%d)\n", buf1, ret1);  // Expected Output: Hello, FAT16!


    
    char buf2[1512];
    for (int i = 0; i < 1512; i++){
        buf2[i] = i % 111;
    }
    fat16_create_file("sample2", "txt", buf2, 1512);

    char buf3[1512];
    int ret2 = fat16_read_file("sample2", "txt", buf3, 1512);
    if (ret2 <= 0) {
        printf("Unable to read file (sample2.txt).\n");
        return -1;
    }

    int mem_ret = memcmp(buf2, buf3, 1512);
    if (mem_ret != 0) {
        printf("Incorrect content of sample2.txt.\n");
        return -1;
    }
    
    fat16_print_root_directory_entries();
    
    return 0;
}