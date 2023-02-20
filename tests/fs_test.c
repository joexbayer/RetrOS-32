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


#define DISKSIZE 200000
#define SMALL_BUFFER_SIZE 1024
#define LARGE_BUFFER_SIZE 4096
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


/* Mock functions */
static FILE* filesystem = NULL;
int kernel_size = 50000;

#define FS_START_LOCATION 0
#define FS_INODE_BMAP_LOCATION FS_START_LOCATION+1
#define FS_BLOCK_BMAP_LOCATION FS_INODE_BMAP_LOCATION+1

int get_current_time(struct time* time_s){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    time_s->year = tm.tm_year;
    time_s->month = tm.tm_mon;
    time_s->day = tm.tm_mday;
    time_s->hour = tm.tm_hour;
    time_s->minute = tm.tm_min;
    time_s->second = tm.tm_sec;

    return 0;
}

void mutex_init(mutex_t* l)
{

}

void acquire(mutex_t* l)
{

}

void release(mutex_t* l)
{

}

int disk_size()
{
    return DISKSIZE;
}


void kfree(void* ptr)
{
    free(ptr);
}

/* Functions needed for inode and bitmap to work. */
int32_t dbgprintf(char* fmt, ...)
{
    if(DEBUG) {
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
    };
    return 0;
}

/* Functions needed for inode and bitmap to work. */
int32_t twriteln(char* fmt, ...)
{
    if(DEBUG) {
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
    }

    return 0;
}

int32_t twritef(char* fmt, ...)
{
    if(DEBUG) {
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
    };
    return 0;
}

void* kalloc(int size){
    return malloc(size);
}

/* Functions simulating the disk device read / write functions. */
int read_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    int ret = fread(buf, 1, 512, filesystem);
    if(ret <= 0)
        return 0;
    return 1;
}

int write_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    fwrite(buf, 1, 512, filesystem);
    return 1;
}

int write_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block(buf, block);
    memcpy(&buf[offset], usr_buf, size);

    return write_block(buf, block);
}

int read_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block((char*)buf, block);
    memcpy(usr_buf, &buf[offset], size);

    return 1;   
}

int main(int argc, char const *argv[])
{

    filesystem = fopen("filesystem.test", "w+");
    init_fs();
    testprintf(1, "Created filesystem.");

    int create_test = fs_create("test.txt");
    testprintf(create_test == 0, "Created test.txt file.");

    fs_mkdir("testdir");
    testprintf(1, "Created test directory.");

    chdir("testdir");
    testprintf(1, "Changed directory to test");


    int create_test2 = fs_create("test2.txt");
    testprintf(create_test2 == 0, "Created test2.txt file in test directory.");
    
    inode_t open_inode = fs_open("test2.txt");
    testprintf(open_inode > 0, "Opened test2.txt");

    int size = fs_size(open_inode);
    testprintf(size == 0, "File test2.txt is empty.");

    char* test_text = "This is a short test string";
    int write_ret = fs_write(test_text, strlen(test_text)+1, open_inode);
    testprintf(write_ret == strlen(test_text)+1, "write test2.txt file.");

    char buffer[2048];
    int read_ret = fs_read(buffer, open_inode);
    testprintf(read_ret == strlen(test_text)+1, "Read test2.txt file.");

    int mem_ret = memcmp(buffer, test_text, strlen(test_text)+1);
    testprintf(mem_ret == 0, "Correct content of test2.txt");

    fs_close(open_inode);
    testprintf(1, "Closed test2.txt");

    printf("TEST - Testing large files with %d max size.\n", MAX_FILE_SIZE);

    int create_large = fs_create("large_file.txt");
    testprintf(create_large == 0, "Created large_file.txt file.");

    inode_t large_inode = fs_open("large_file.txt");
    testprintf(large_inode > 0, "Opened large_file.txt");

    char small_buffer[SMALL_BUFFER_SIZE];
    for (size_t i = 0; i < SMALL_BUFFER_SIZE; i++) small_buffer[i] = i % 111;
    
    int small_write = fs_write(small_buffer, SMALL_BUFFER_SIZE, large_inode);
    testprintf(small_write == SMALL_BUFFER_SIZE, "Wrote bytes to large_file.txt");

    char small_read_buffer[SMALL_BUFFER_SIZE];
    int small_read = fs_read(small_read_buffer, large_inode);
    testprintf(small_read == SMALL_BUFFER_SIZE, "Read bytes from large_file.txt");
    
    int small_mem_ret = memcmp(small_read_buffer, small_buffer, SMALL_BUFFER_SIZE);
    testprintf(small_mem_ret == 0, "Correct content of large_file.txt");

    fs_close(large_inode);

    int create_xlarge = fs_create("xlarge_file.txt");
    testprintf(create_xlarge == 0, "Created xlarge_file.txt file.");

    large_inode = fs_open("xlarge_file.txt");
    testprintf(large_inode > 0, "Opened xlarge_file.txt");

    char* large_buffer = malloc(LARGE_BUFFER_SIZE);
    for (short i = 0; i < LARGE_BUFFER_SIZE; i++) large_buffer[i] = i % 111;

    int large_write = fs_write(large_buffer, LARGE_BUFFER_SIZE, large_inode);
    testprintf(large_write == LARGE_BUFFER_SIZE, "Wrote bytes to extra_large_file.txt");

    char* large_read_buffer = malloc(LARGE_BUFFER_SIZE);
    int large_read = fs_read(large_read_buffer, large_inode);
    testprintf(large_read == LARGE_BUFFER_SIZE, "Read bytes from extra_large_file.txt");

    int large_mem_ret = memcmp(large_read_buffer, large_buffer, LARGE_BUFFER_SIZE);
    testprintf(large_mem_ret == 0, "Correct content of extra_large_file.txt");

    free(large_buffer);
    free(large_read_buffer);
    fs_close(large_inode);

    fclose(filesystem);
    /* code */
    return 0;
}
