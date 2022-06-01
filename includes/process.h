#ifndef PROCESS_H
#define PROCESS_H

#include <util.h>

#define MAX_FUNCTION_NAME 10
#define MAX_PROCESS_NAME 10
#define MAX_PROCESS_FUNCTIONS 5

#define MAX_PROCESS_INSTANCES 5

extern struct process* current_process;

enum {
    FOCUS,
    NOFOCUS
};

struct process_function {
    char name[MAX_FUNCTION_NAME];
    void (*f)();
};

struct process {
    char name[MAX_PROCESS_NAME];
    struct process_function functions[MAX_PROCESS_FUNCTIONS];
    int total_functions;
    void (*entry)();

    int16_t pid;
    uint16_t focus;

    int16_t instances[MAX_PROCESS_INSTANCES];
};

int ATTACH_FUNCTION(int pid, char* name, void (*fn)());
int ATTACH_PROCESS(char* name, void (*entry)());

void list_processes();
void start_process(int id);
void list_instances();
void switch_process(int id);
void list_functions();




#endif /* PROCESS_H */
