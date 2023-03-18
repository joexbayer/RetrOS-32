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

#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/superblock.h>
#include <fs/directory.h>

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


#define SMALL_BUFFER_SIZE 1024
#define LARGE_BUFFER_SIZE 25096
#define DEBUG 0


int test_count = 0;
int failed = 0;
void testprintf(int test,  const char* test_str)
{
    if(test)
        fprintf(stderr, "TEST [ " GREEN "OK" RESET " ] %s\n", test_str);
    else {
        fprintf(stderr, "TEST [ " RED "FAILED" RESET " ] %s\n", test_str);
        failed++;
    }
}

struct pcb {
    inode_t current_directory;
} cur = {
    .current_directory = 1
};
extern struct pcb* current_running = &cur;


/* Mock functions */
FILE* filesystem = NULL;
int kernel_size = 50000;

#define FS_START_LOCATION 0
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

void test_file_size(int size)
{
    printf("Testing to create and write file of size %d\n", size);

    int create_test2 = fs_create("xlarge_file.txt");
    testprintf(create_test2 == 0, "Created xlarge_file.txt file.");

    int large_inode = fs_open("xlarge_file.txt");
    testprintf(large_inode > 0, "Opened xlarge_file.txt");

    char* large_buffer = malloc(size);
    for (short i = 0; i < size; i++) large_buffer[i] = i % 111;

    int large_write = fs_write(large_inode, large_buffer, size);
    testprintf(large_write == size, "Wrote bytes to file.txt");

    char* large_read_buffer = malloc(size);
    fs_seek(large_inode, 0, 0);
    int large_read = fs_read(large_inode, large_read_buffer, size);
    testprintf(large_read == size, "Read bytes from extra_large_file.txt");

    int large_mem_ret = memcmp(large_read_buffer, large_buffer, size);
    testprintf(large_mem_ret == 0, "Correct content of extra_large_file.txt");

    free(large_buffer);
    free(large_read_buffer);
    fs_close(large_inode);
}

int main(int argc, char const *argv[])
{

    filesystem = fopen("filesystem.test", "w+");
    init_fs();
    testprintf(1, "Created filesystem.");

    current_running->current_directory = fs_get_root();
    printf("ROOT: %d\n", current_running->current_directory);

    int create_test = fs_create("test.txt");
    testprintf(create_test == 0, "Created test.txt file.");

    fs_mkdir("testdir", fs_get_root());
    testprintf(1, "Created test directory.");

    int create_test2 = fs_create("test2.txt");
    testprintf(create_test2 == 0, "Created test2.txt file in test directory.");
    
    inode_t open_inode = fs_open("test2.txt");
    testprintf(open_inode > 0, "Opened test2.txt");

    int size = fs_size(open_inode);
    testprintf(size == 0, "File test2.txt is empty.");

    char* test_text = "This is a short test string";
    int write_ret = fs_write(open_inode, test_text, strlen(test_text)+1);
    testprintf(write_ret == strlen(test_text)+1, "write test2.txt file.");

    char buffer[2048];
    fs_seek(open_inode, 0, 0);
    int read_ret = fs_read(open_inode, buffer, 2048);
    testprintf(read_ret == strlen(test_text)+1, "Read test2.txt file.");

    int mem_ret = memcmp(buffer, test_text, strlen(test_text)+1);
    testprintf(mem_ret == 0, "Correct content of test2.txt");

    fs_close(open_inode);
    testprintf(1, "Closed test2.txt");

    printf("TEST - Testing large files with %d max size.\n", MAX_FILE_SIZE);

    test_file_size(SMALL_BUFFER_SIZE);
    test_file_size(LARGE_BUFFER_SIZE);
    //test_file_size(MAX_FILE_SIZE);

    fclose(filesystem);
    /* code */
    return 0;
}
