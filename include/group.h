#ifndef __GROUP_H
#define __GROUP_H

#include <kernel.h>
#include <kutils.h>

typedef int gid_t;

struct group {
    gid_t gid;
    int permissions;
    char name[32];
    char password[32];
};

#endif // !__GROUP_H