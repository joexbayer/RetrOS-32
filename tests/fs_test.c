#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

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

int disk_size()
{
    return DISKSIZE;
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

void* alloc(int size){
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

    ls("");

    fcreate("test.txt");
    testprintf(1, "Created test.txt file.");

    ls("");

    fs_mkdir("testdir");
    testprintf(1, "Created test directory.");

    ls("");
    chdir("testdir");
    testprintf(1, "Changed directory to test");
    ls("");


    fcreate("test2.txt");
    testprintf(1, "Created test2.txt file in test directory.");
    ls("");
    
    inode_t open_inode = fs_open("test2.txt");
    testprintf(open_inode > 0, "Opened test2.txt");

    ls("");
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



    fclose(filesystem);
    /* code */
    return 0;
}
