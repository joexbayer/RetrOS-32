#include <conf.h>
#include <fs/fs.h>

struct config __config = {
    .sections = NULL,
    .section_count = 0,
    .loaded = false
};

int load_config(char* filename)
{
    int ret;
    ubyte_t* buf = kalloc(1024);
    ret = fs_load_from_file(filename, buf, 1024);
    if(ret <= 0){
        kfree(buf);
        return ret;
    }



}


char* config_get_value(char* section, char* name);