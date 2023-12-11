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

int kenv_init(struct kernel_environment* env)
{
    env->size = 0;
    return 0;
}

int kenv_set(struct kernel_environment* env, char* name, char* value)
{
    if(env->size >= 100) return -1;

    struct environment_variable* var = &env->variables[env->size];
    var->name = name;
    var->value = value;
    env->size++;
    return 0;
}
