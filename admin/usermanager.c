#include <admin.h>
#include <memory.h>
#include <kernel.h>
#include <fs/fs.h>

#include <terminal.h>

#include <usermanager.h>
#include <user.h>
#include <group.h>
#include <admin.h>
#include <ksyms.h>

#include <errors.h>

#include <kernel.h>

#define USRMAN_MAGIC 0x55DB

#define IS_LOADED(usrman) ((usrman)->db.magic == USRMAN_MAGIC)

/* prototypes */
static int __add_user(struct usermanager* usrman, struct user* user);
static int __remove_user(struct usermanager* usrman, struct user* user);
static int __change_user(struct usermanager* usrman, struct user* user, permission_t permission);
static int __list_users(struct usermanager* usrman);
static int __load_users(struct usermanager* usrman);
static int __save_users(struct usermanager* usrman);
static struct user* __authenticate_user(struct usermanager* usrman, const char* username, const char* password);

/* consts */
static const char* userdb = "/sysutil/users.db";
static struct user_manager_ops default_ops = {
	.add = __add_user,
	.remove = __remove_user,
	.change = __change_user,
	.list = __list_users,
	.load = __load_users,
	.save = __save_users,
	.authenticate = __authenticate_user
};
static const struct userdb default_db = {
	.magic = USRMAN_MAGIC,
	.users = {
			{
			.name = "admin",
			.hash = 0,
			.uid = 1,
			.permissions = _
		}
	},
	.groups = {
		{
			.name = "admin",
			.gid = 1,
			.permissions = _
		}
	}
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


static struct user* __find_user(struct usermanager* usrman, const char* username)
{
	ERR_ON_NULL_PTR(usrman);
	ERR_ON_NULL_PTR(username);

	if(!IS_LOADED(usrman)){
		return NULL;
	}


	for(int i = 0; i < 8; i++){
		if(strcmp(usrman->db.users[i].name, username) == 0){
			return &usrman->db.users[i];
		}
	}

	return NULL;
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
		usrman->db = default_db;
		dbgprintf("Failed to load userdb from file: using default.\n");
		return -1;
	}

	dbgprintf("Loaded userdb from file.\n");

	return 0;
}

static int __save_users(struct usermanager* usrman)
{
	int ret;
	ERR_ON_NULL(usrman);

	if(!IS_LOADED(usrman)){
		return -1;
	}


	ret = fs_save_to_file(userdb, &usrman->db, sizeof(usrman->db));
	if(ret < 0){
		dbgprintf("Failed to save userdb to file\n");
		return -1;
	}

	return 0;
}

static struct user* __authenticate_user(struct usermanager* usrman, const char* username, const char* password)
{
	ERR_ON_NULL_PTR(usrman);
	ERR_ON_NULL_PTR(username);
	ERR_ON_NULL_PTR(password);

	if(!IS_LOADED(usrman)){
		return NULL;
	}


	struct user* user = __find_user(usrman, username);
	if(user == NULL){
		return NULL;
	}

	unsigned int hash = advanced_hash(password);
	if(hash == user->hash){
		return user;
	}
	
	return NULL;
}

static int __add_user(struct usermanager* usrman, struct user* user)
{
	ERR_ON_NULL(usrman);
	ERR_ON_NULL(user);

	if(!IS_LOADED(usrman)){
		return -1;
	}


	/* TBD */
	
	return -1;
}

static int __list_users(struct usermanager* usrman)
{
	ERR_ON_NULL(usrman);

	if(!IS_LOADED(usrman)){
		return -1;
	}


	for(int i = 0; i < 8; i++){
		if(usrman->db.users[i].uid == 0){
			continue;
		}
		twritef("%s\n", usrman->db.users[i].name);
	}

	return 0;
}

static int __remove_user(struct usermanager* usrman, struct user* user)
{
	ERR_ON_NULL(usrman);
	ERR_ON_NULL(user);

	if(!IS_LOADED(usrman)){
		return -1;
	}

	/* TBD */
	
	return -1;
}

static int __change_user(struct usermanager* usrman, struct user* user, permission_t permission)
{
	ERR_ON_NULL(usrman);
	ERR_ON_NULL(user);

	if(!IS_LOADED(usrman)){
		return -1;
	}

	/* TBD */
	
	return -1;
}

static int admin(int argc, char** argv)
{
	/* list, create, remove, add */
	$services->user_manager->ops->list($services->user_manager);

	return -1;
}
EXPORT_KSYMBOL(admin);