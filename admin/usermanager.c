#include <admin.h>
#include <memory.h>
#include <fs/fs.h>

#include <errors.h>

#define USRMAN_MAGIC 0x55DB

#define IS_LOADED(usrman) ((usrman)->db.magic == USRMAN_MAGIC)

/* prototypes */
static int __add_user(struct usermanager* usrman, struct user* user);
static int __remove_user(struct usermanager* usrman, struct user* user);
static int __change_user(struct usermanager* usrman, struct user* user, permission_t permission);
static int __list_users(struct usermanager* usrman);
static int __load_users(struct usermanager* usrman);
static int __save_users(struct usermanager* usrman);

/* consts */
static const char* userdb = "/sysutil/users.db";
static const struct user_manager_ops default_ops = {
	.add = __add_user,
	.remove = __remove_user,
	.change = __change_user,
	.list = __list_users
};

/* Main access point */
struct usermanager* usermanager_create()
{
	struct usermanager* usrman = create(struct usermanager);
	if(usrman == NULL){
		return NULL;
	}
	usrman->ops = &default_ops;

	return usrman;
}

/* ops implementations */
static int __load_users(struct usermanager* usrman)
{
	int ret;
	ERR_ON_NULL(usrman);

	if(usrman->db.magic == USRMAN_MAGIC){
		return -1;
	}

	ret = fs_load_from_file(userdb, &usrman->db, sizeof(usrman->db));
	if(ret < 0){
		dbgprintf("Failed to load userdb from file\n");
		return -1;
	}

	return 0;
}

static int __save_users(struct usermanager* usrman)
{
	int ret;
	ERR_ON_NULL(usrman);

	if(usrman->db.magic != USRMAN_MAGIC){
		return -1;
	}

	ret = fs_save_to_file(userdb, &usrman->db, sizeof(usrman->db));
	if(ret < 0){
		dbgprintf("Failed to save userdb to file\n");
		return -1;
	}

	return 0;
}

static int __add_user(struct usermanager* usrman, struct user* user)
{
	ERR_ON_NULL(usrman);
	ERR_ON_NULL(user);

	IS_LOADED(usrman);

	/* TBD */
	
	return -1;
}
