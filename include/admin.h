#ifndef __ADMIN_H
#define __ADMIN_H

#include <kernel.h>
#include <kutils.h>

#include <user.h>

typedef enum __permissions {
    _ = 0 << 0,
} permission_t;

struct usermanager;

struct user_manager_ops {
    int (*add)(struct usermanager *manager, struct user *user);
    int (*remove)(struct usermanager *manager, struct user *user);
    int (*change)(struct usermanager *manager, struct user *user, permission_t permissions);
    int (*list)(struct usermanager *manager);
};

struct usermanager {
    struct user_manager_ops *ops;
    struct user users[8];
    struct group groups[8];
};

struct usermanager* usermanager_create();

#endif // !__ADMIN_H