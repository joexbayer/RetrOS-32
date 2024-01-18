#ifndef F2B1F79B_0C15_4F82_B78A_5ABEC923D093
#define F2B1F79B_0C15_4F82_B78A_5ABEC923D093

#include <kernel.h>
#include <user.h>
#include <group.h>
#include <stdint.h>
#include <admin.h>

#define IS_AUTHORIZED(user, permission) ((user)->permissions & (permission))
#define AUTHORIZED_GUARD(permission) if (!IS_AUTHORIZED($process->current->user, permission)) return ERR_ACCESS_DENIED

struct usermanager;

struct user_manager_ops {
    int (*add)(struct usermanager *manager, struct user *user);
    int (*remove)(struct usermanager *manager, struct user *user);
    int (*change)(struct usermanager *manager, struct user *user, permission_t permissions);
    int (*list)(struct usermanager *manager);
    int (*load)(struct usermanager *manager);
    int (*save)(struct usermanager *manager);
    struct user* (*authenticate)(struct usermanager *manager, const char* username, const char* password);
};

struct usermanager {
    struct user_manager_ops *ops;
    struct userdb {
        uint32_t magic;
        struct user users[8];
        struct group groups[8];
    } db;
};

struct usermanager* usermanager_create();

#endif /* F2B1F79B_0C15_4F82_B78A_5ABEC923D093 */
