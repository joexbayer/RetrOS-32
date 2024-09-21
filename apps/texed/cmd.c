
#ifndef NCURSES
#include <libc.h>
#include <lib/syscall.h>
#else
#include <ncurses.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#endif // !NCURSES

#include "include/screen.h"
#include "include/textbuffer.h"

#define MAX_CMD 100

static int textbuffer_replace(struct textbuffer *buffer, char *search, char *replace) {
    for (size_t i = 0; i < buffer->line_count; i++) {
        int index;
        while ((index = strstr(buffer->lines[i]->text, search)) >= 0) {
            size_t replace_len = strlen(replace);
            size_t search_len = strlen(search);
            size_t new_length = buffer->lines[i]->length - search_len + replace_len;

            if (new_length > buffer->lines[i]->capacity) {
                return -1;
            }

            memmove(buffer->lines[i]->text + index + replace_len,
                    buffer->lines[i]->text + index + search_len,
                    buffer->lines[i]->length - index - search_len);
            memcpy(buffer->lines[i]->text + index, replace, replace_len);

            buffer->lines[i]->length = new_length;
        }
    }
    return 0;
}

static int textbuffer_parse_replace(struct textbuffer *buffer, char *command) {
    char search[MAX_CMD];
    char replace[MAX_CMD];
    int i = 0;
    int j = 0;
    int k = 0;
    while(command[i] != ' ') {
        i++;
    }
    i++;
    while (command[i] != ' ') {
        search[j] = command[i];
        i++;
        j++;
    }
    search[j] = '\0';
    i++;
    while (command[i] != '\0') {
        replace[k] = command[i];
        i++;
        k++;
    }
    replace[k] = '\0';

    textbuffer_replace(buffer, search, replace);
}

static int textbuffer_command(struct textbuffer* buffer, char* command) {
    if (strcmp(command, "search") == 0) {
        textbuffer_search_main(buffer);
    }

    if (strcmp(command, "exit") == 0) {
        return -117;
    }
    
    // replace word with word without strtok
    if (strncmp(command, "replace", 7) == 0) {
        textbuffer_parse_replace(buffer, command);
    }

    return 0;
}


int textbuffer_command_main(struct textbuffer* buffer) {
    textbuffer_get_input(buffer, 'C', "Command: ", textbuffer_command);
    return 0;
}