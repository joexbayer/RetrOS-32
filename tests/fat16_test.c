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

    
    print_root_directory();
    
    return 0;
}