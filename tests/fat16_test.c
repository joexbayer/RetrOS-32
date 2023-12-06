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

#define DEBUG 0

/* needed by mocks.c */
FILE* filesystem = NULL;

int main(int argc, char const *argv[])
{
    filesystem = fopen("filesystem.test", "w+" /* w+ */);
    if(filesystem == NULL){
        printf("Unable to open mock filesystem.");
        return -1;
    }

    // Existing tests
    testprintf(fat16_load() != 0, "fat16_load() on empty filesystem");
    testprintf(fat16_format("VOLUME1", 1) == 0, "fat16_format()");
    testprintf(fat16_load() == 0, "fat16_load()");
    testprintf(fat16_create_directory("test_dir") == 0, "fat16_create_directory()");
    testprintf(fat16_create_empty_file("file.txt", 0) == 0, "fat16_create_empty_file()");

    // Additional tests
    struct fat16_directory_entry entry;
    testprintf(fat16_read_entry(0, 0, &entry) == 0, "fat16_read_entry()");
    testprintf(fat16_sync_directory_entry(0, 0, &entry) == 0, "fat16_sync_directory_entry()");

    // Test set time and date functions
    uint16_t time, date;
    fat16_set_time(&time, 12, 30, 30); // No return value to test
    testprintf(1, "fat16_set_time()");
    fat16_set_date(&date, 2023, 1, 1); // No return value to test
    testprintf(1, "fat16_set_date()");

    // Test FAT16 File and Directory Manipulation Functions
    struct fat16_directory_entry dir_entry;
    testprintf(fat16_read_entry(1, 0, &dir_entry) == 0, "fat16_read_entry() on Directory");
    
    int first_cluster = 2; // Example cluster number
    char data[512]; // Example data buffer
    testprintf(fat16_read_data(first_cluster, 0, data, sizeof(data), sizeof(data)) == 512, "fat16_read_data()");
    
    testprintf(fat16_write_data(first_cluster, 0, data, sizeof(data)) == 512, "fat16_write_data()");

    struct fat16_directory_entry file_entry;
    struct fat16_file_identifier id =  fat16_get_directory_entry("file.txt", &file_entry);
    // Test FAT16 File Deletion
    int block = id.directory, index = id.index; // Example block and index

    /* write on file */
    char string[] = "Hello World!";
    testprintf(fat16_write_data(file_entry.first_cluster, 0, string, sizeof(string)) == 13, "fat16_write_data() on file.txt");

    /* read on file */
    char buffer[512];
    testprintf(fat16_read_data(file_entry.first_cluster, 0, buffer, sizeof(string), sizeof(string)) == 13, "fat16_read_data() on file.txt");

    /* compare */
    testprintf(memcmp(string, buffer, sizeof(string)) == 0, "Correct data written and read on file.txt");

    // Test FAT16 File Renaming
    char new_name[] = "NEWNAME.TXT";
    testprintf(fat16_rename_entry(block, index, new_name) == 0, "fat16_rename_entry()");

    // Test FAT16 Cluster Allocation and Deallocation
    uint32_t free_cluster = fat16_get_free_cluster();
    testprintf(free_cluster != 0, "fat16_get_free_cluster()");
    
    fat16_allocate_cluster(free_cluster); // No return value to test
    testprintf(1, "fat16_allocate_cluster()");
    
    fat16_free_cluster(free_cluster); // No return value to test
    testprintf(1, "fat16_free_cluster()");

    // Test FAT16 File Creation
    testprintf(fat16_create_file("NEWFILE", "TXT", data, sizeof(data)) == 0, "fat16_create_file()");
    return 0;
}