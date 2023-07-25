#include <kutils.h>
#include <util.h>

struct environment_variable {
    char* name;
    char* value;
};

struct kernel_environment {
    struct environment_variable variables[100];
    int size;
};

