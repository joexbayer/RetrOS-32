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

#include <fs/ext.h>
#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>
#include <mocks.h>
#include <test.h>

#define SMALL_BUFFER_SIZE 1024
#define LARGE_BUFFER_SIZE 4*4096-8
#define DEBUG 0

/* Mock functions */
FILE* filesystem = NULL;
int kernel_size = 50000;

#define FS_START_LOCATION 0
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

void test_file_size(int size)
{
    printf("Testing to create and write file of size %d\n", size);

    int create_test2 = ext_create("xlarge_file.txt");
    testprintf(create_test2 == 0, "Created xlarge_file.txt file.");

    int large_inode = ext_open("/xlarge_file.txt", 0);
    testprintf(large_inode > 0, "Opened xlarge_file.txt");

    char* large_buffer = malloc(size);
    for (short i = 0; i < size; i++) large_buffer[i] = i % 111;

    int large_write = ext_write(large_inode, large_buffer, size);
    testprintf(large_write == size, "Wrote bytes to file.txt");

    char* large_read_buffer = malloc(size);
    ext_seek(large_inode, 0, 0);
    int large_read = ext_read(large_inode, large_read_buffer, size);
    testprintf(large_read == size, "Read bytes from extra_large_file.txt");

    int large_mem_ret = memcmp(large_read_buffer, large_buffer, size);
    testprintf(large_mem_ret == 0, "Correct content of extra_large_file.txt");

    free(large_buffer);
    free(large_read_buffer);
    ext_close(large_inode);
}

int main(int argc, char const *argv[])
{

    filesystem = fopen("filesystem.test", "w+");
    ext_create_file_system();
    testprintf(1, "Created filesystem.");

    int create_test = ext_create("test.txt");
    testprintf(create_test == 0, "Created test.txt file.");

    ext_create_directory("testdir", ext_get_root());
    testprintf(1, "Created test directory.");

    int create_test2 = ext_create("test2.txt");
    testprintf(create_test2 == 0, "Created test2.txt file in test directory.");
    
    inode_t open_inode = ext_open("/test2.txt", 0);
    testprintf(open_inode > 0, "Opened test2.txt");

    int size = ext_size(open_inode);
    testprintf(size == 0, "File test2.txt is empty.");

    char* test_text = "This is a short test string";
    int write_ret = ext_write(open_inode, test_text, strlen(test_text)+1);
    testprintf(write_ret == strlen(test_text)+1, "write test2.txt file.");

    char buffer[2048];
    ext_seek(open_inode, 0, 0);
    int read_ret = ext_read(open_inode, buffer, 2048);
    testprintf(read_ret == strlen(test_text)+1, "Read test2.txt file.");

    int mem_ret = memcmp(buffer, test_text, strlen(test_text)+1);
    testprintf(mem_ret == 0, "Correct content of test2.txt");

    ext_close(open_inode);
    testprintf(1, "Closed test2.txt");

    printf("TEST - Testing large files with %d max size.\n", MAX_FILE_SIZE);

    test_file_size(SMALL_BUFFER_SIZE);
    test_file_size(LARGE_BUFFER_SIZE);
    //test_file_size(MAX_FILE_SIZE);

    fclose(filesystem);
    /* code */
    return failed > 0 ? -1 : 0;
}
