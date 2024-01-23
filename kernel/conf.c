/**
 * @file conf.c
 * @author Joe Bayer (joexbayer)
 * @brief Configuration file parser.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <conf.h>
#include <fs/fs.h>
#include <memory.h>
#include <serial.h>
#include <terminal.h>

static struct config {
    struct config_section {
        char section_name[32];
        struct config_entry {
            char key[32];
            char value[32];
        } entries[16];
        int entry_count;
    } sections[16];
    int section_count;

    bool_t loaded;
} __config = {
    .loaded = false,
};

static int config_clear()
{
    memset(&__config, 0, sizeof(struct config));
    return 0;
}

static int config_add_section(char* name)
{
    if(__config.section_count >= 16){
        return -1;
    }
    struct config_section* section = &__config.sections[__config.section_count];
    strcpy(section->section_name, name);
    section->entry_count = 0;
    __config.section_count++;
    return 0;
}

static int config_add_entry(char* section, char* key, char* value)
{
    struct config_section* sec = NULL;
    for(int i = 0; i < __config.section_count; i++){
        if(strcmp(__config.sections[i].section_name, section) == 0){
            sec = &__config.sections[i];
            break;
        }
    }

    if(sec == NULL){
        return -1;
    }

    if(sec->entry_count >= 16){
        return -1;
    }

    struct config_entry* entry = &sec->entries[sec->entry_count];
    strcpy(entry->key, key);
    strcpy(entry->value, value);
    sec->entry_count++;
    
    return 0;
}

int config_list()
{
    for(int i = 0; i < __config.section_count; i++){
        struct config_section* sec = &__config.sections[i];
        twritef("[%s]\n", sec->section_name);
        for(int j = 0; j < sec->entry_count; j++){
            struct config_entry* entry = &sec->entries[j];
            twritef("%s = %s\n", entry->key, entry->value);
        }
    }
    return 0;
}

int kernel_config_load(char* filename)
{
    int len;
    char* buf = kalloc(1024);
    len = fs_load_from_file(filename, buf, 1024);
    if(len <= 0){
        kfree(buf);
        return len;
    }
    buf[len] = '\0';

    config_clear();

    char* line = buf;
    char* section = NULL;

    while (*line != '\0' && (line - buf) < len) {
        /* Skip leading newlines */
        while (*line == '\n' && (line - buf) < len) line++;

        /* Parse section */
        if (*line == '[') {
            section = line + 1;
            while (*line != ']' && *line != '\0' && (line - buf) < len) {
                line++;
            }
            if (*line == ']') {
                *line = '\0';
                line++;
                config_add_section(section);
            } else {
                /* Malformed section, skip to next line */
                section = NULL;
                while (*line != '\n' && *line != '\0' && (line - buf) < len) line++;
                continue;
            }
        }

        /* Skip comments */
        if (*line == '#') {
            while (*line != '\n' && *line != '\0' && (line - buf) < len) {
                line++;
            }
            continue;
        }

        /* Parse key-value pair */
        if (section != NULL) { /* Ensure we are within a section */
            char* name = (char*)line;
            while (*line != '=' && *line != '\n' && *line != '\0' && (line - buf) < len) {
                line++;
            }
            if (*line == '=') {
                *line = '\0';
                line++;
                char* value = (char*)line;
                while (*line != '\n' && *line != '\0' && (line - buf) < len) {
                    line++;
                }
                if (*line != '\0') {
                    *line = '\0';
                    line++;
                }
                config_add_entry(section, name, value);
                dbgprintf("config: %s.%s = %s\n", section, name, value);
            }
        }
    }


    return 0;
}


char* config_get_value(char* section, char* name)
{
    for(int i = 0; i < __config.section_count; i++){
        struct config_section* sec = &__config.sections[i];
        if(strcmp(sec->section_name, section) == 0){
            for(int j = 0; j < sec->entry_count; j++){
                struct config_entry* entry = &sec->entries[j];
                if(strcmp(entry->key, name) == 0){
                    return entry->value;
                }
            }
        }
    }
    return NULL;
}