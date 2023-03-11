#include <pcb.h>
#include <stdio.h>

FILE* filesystem = NULL;
unsigned int* kernel_page_dir = 0;
int cli_cnt = 0;
int kernel_size = 50000;

int main(int argc, char const *argv[])
{
    pcb_init();
    return 0;
}