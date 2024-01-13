#ifndef __USER_H__
#define __USER_H__

#include <kernel.h>
#include <kutils.h>

#include <group.h>
#include <admin.h>

typedef int uid_t;

struct user {
    uid_t uid;
    char name[32];
    unsigned int hash;
    permission_t permissions;
    
};


#endif // !__USER_H__

#include <kutils.h>

