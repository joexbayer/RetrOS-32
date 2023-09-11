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

#include <mocks.h>

#define DEBUG 1

/* needed by mocks.c */
FILE* filesystem = NULL;

int main(int argc, char const *argv[])
{
    filesystem = fopen("filesystem.test", "w+" /* w+ */);
    if(filesystem == NULL){
        printf("Unable to open mock filesystem.");
        return -1;
    }

    fat16_format("VOLUME1", 1);
    if(fat16_load() < 0){
        printf("Unable to initialize FAT16 filesystem.\n");
        return -1;
    }

    fat16_create_file("FILENAME", "TXT", "Hello, FAT16!", strlen("Hello, FAT16!")+1);
    char buf1[3000];
    int ret1 = fat16_read_file("FILENAME", "TXT", buf1, 14);
    if (ret1 <= 0) {
        printf("Unable to read file (sample.txt).\n");
    }
    printf("%s (%d)\n", buf1, ret1);  // Expected Output: Hello, FAT16!

    // int size = 8*1024*1024;
    
    // char* buf2 = malloc(size);
    // for (int i = 0; i < size; i++){
    //     buf2[i] = i % 111;
    // }
    // fat16_create_file("sample2", "txt", buf2, size);

    // char* buf3 = malloc(size);
    // int ret2 = fat16_read_file("sample2", "txt", buf3, size);
    // if (ret2 <= 0) {
    //     printf("Unable to read file (sample2.txt).\n");
    //     return -1;
    // }

    // int mem_ret = memcmp(buf2, buf3, size);
    // if (mem_ret != 0) {
    //     printf("Incorrect content of sample2.txt.\n");
    //     return -1;
    // }
    
    fat16_create_directory("DIR     ");

    fat16_print_root_directory_entries();

    fat16_change_directory("DIR     ");

    fat16_create_directory("DIR2    ");

    fat16_print_root_directory_entries();

    fat16_change_directory("DIR2    ");

    fat16_create_file("FILENAM2", "TXT", "Hello, FAT16!", strlen("Hello, FAT16!")+1);

    fat16_print_root_directory_entries();

    char* path = "/DIR/DIR2/FILENAM2.TXT";   

    struct fat16_directory_entry entry;
    struct fat16_file_identifier file_id;

    file_id = fat16_get_directory_entry(path, &entry);
    printf("File ID: %d : %d\n", file_id.directory, file_id.index);
    fat16_directory_entry_debug(&entry);

    file_id = fat16_get_directory_entry("/", &entry);
    printf("File ID: %d : %d\n", file_id.directory, file_id.index);
    fat16_directory_entry_debug(&entry);

    file_id = fat16_get_directory_entry("/DIR", &entry);
    printf("File ID: %d : %d\n", file_id.directory, file_id.index);
    fat16_directory_entry_debug(&entry);


    fseek(filesystem, 0, SEEK_END);
    int size2 = ftell(filesystem);
    printf("Size of filesystem: %d\n", size2);
    /* pad to 32mb */
    if(size2 < 32*1024*1024){
        char* buf = malloc(32*1024*1024 - size2);
        memset(buf, 0, 32*1024*1024 - size2);
        fwrite(buf, 32*1024*1024 - size2, 1, filesystem);
        free(buf);

        printf("Padded filesystem to 32mb.\n");
    }

    fat16_sync_fat_table();
    
    return 0;
}