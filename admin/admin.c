#include <user.h>
#include <group.h>
#include <admin.h>

static struct user __default_admin __attribute__((unused)) = {
    .name = "admin",
    .hash = 0,
    .uid = 1,
    .permissions = _
};


