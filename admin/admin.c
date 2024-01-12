#include <user.h>
#include <group.h>
#include <admin.h>

static struct user __default_admin = {
    .name = "admin",
    .password = "admin",
    .uid = 1,
    .permissions = _
};



