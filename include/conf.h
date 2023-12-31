#ifndef __KCONF_H__
#define __KCONF_H__

#include <stdint.h>
#include <kernel.h>
#include <kutils.h>
#include <util.h>

struct config_entry {
    char* key;
    char* value;
};

struct config_section {
    char* section_name;
    struct config_entry* entries;
    int entry_count;
};

struct config {
    struct config_section* sections;
    int section_count;

    bool_t loaded;
};

int load_config(char* filename);
char* config_get_value(char* section, char* name);


#endif /* __KCONF_H__ */