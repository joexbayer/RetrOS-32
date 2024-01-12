#include <admin.h>

#include <memory.h>

static int __add_user(struct usermanager* usrman, struct user* user);
static int __remove_user(struct usermanager* usrman, struct user* user);
static int __change_user(struct usermanager* usrman, struct user* user, permission_t permission);
static int __list_users(struct usermanager* usrman);

static struct user_manager_ops default_ops = {
    .add = __add_user,
    .remove = __remove_user,
    .change = __change_user,
    .list = __list_users
};

struct usermanager* usermanager_create()
{
    struct usermanager* usrman = create(struct usermanager);
    if(usrman == NULL){
        return NULL;
    }
    usrman->ops = &default_ops;

    return usrman;
}

static int __add_user(struct usermanager* usrman, struct user* user)
{
    if(usrman == NULL || user == NULL){
        return -1;
    }
    for(int i = 0; i < 8; i++){
        if(usrman->users[i].uid == 0){
            usrman->users[i] = *user;
            return 0;
        }
    }
    return -1;
}
