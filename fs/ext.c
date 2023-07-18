#include <fs/ext.h>
#include <fs/ext_error.h>
#include <fs/superblock.h>
#include <fs/path.h>
#include <fs/inode.h>
#include <fs/directory.h>
#include <memory.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>
#include <serial.h>
#include <pcb.h>
#include <ksyms.h>

#include <util.h>
#include <rtc.h>

#include <editor.h>

static struct superblock superblock;
static struct inode* root_dir;
static struct inode* current_dir;

static int FS_START_LOCATION = 0;
static int FS_INODE_BMAP_LOCATION = 0;
static int FS_BLOCK_BMAP_LOCATION = 0;

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

#define CHECK_DISK() if(!disk_attached()){\
		dbgprintf("[FS]: No disk device found.\n");\
		return -1;\
	}

int init_ext()
{
	FS_START_LOCATION = (kernel_size/512)+2;
	FS_INODE_BMAP_LOCATION = FS_START_LOCATION+1;
	FS_BLOCK_BMAP_LOCATION = FS_INODE_BMAP_LOCATION+1;

	/* Read superblock and check magic. */
	read_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
	if(superblock.magic != MAGIC){
		ext_create_file_system();
		return 1;
	}

	dbgprintf("[FS]: Found Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
	dbgprintf("[FS]: With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
	dbgprintf("[FS]: And total of %d block\n", superblock.nblocks);
	dbgprintf("[FS]: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);

	superblock.inodes_start = FS_START_LOCATION + 3;
	superblock.blocks_start = superblock.inodes_start + (superblock.ninodes/ INODES_PER_BLOCK);

	superblock.block_map = create_bitmap(superblock.nblocks);
	superblock.inode_map = create_bitmap(superblock.ninodes);
	read_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);
	read_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);


	root_dir = inode_get(superblock.root_inode, &superblock);
	dbgprintf("[FS]: Root inode: %d\n", superblock.root_inode);
	root_dir->nlink++;
	current_dir = root_dir;

	return 0;
}

inode_t ext_get_root()
{
	return root_dir->inode;
}

inode_t ext_get_current_dir()
{
	return current_running->current_directory;
}


int ext_get_size()
{
	return superblock.size;
}

int ext_get_inodes()
{
	return superblock.ninodes;
}

int ext_get_blocks()
{
	return superblock.nblocks;
}

void ext_sync()
{
	inodes_sync(&superblock);
	write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
	write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
	write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);
}

static inline void __inode_add_dir(struct directory_entry* entry, struct inode* inode, struct superblock* sb)
{
	inode->pos = inode->size;
	inode_write((char*) entry, sizeof(struct directory_entry), inode, sb);
}

void ext_create_file_system()
{
	superblock.magic = MAGIC;
	superblock.size = (disk_size()) - (FS_START_LOCATION*BLOCK_SIZE);

	superblock.ninodes = (superblock.size / (sizeof(struct inode)+NDIRECT*BLOCK_SIZE));
	superblock.nblocks = superblock.ninodes*NDIRECT;

	superblock.inodes_start = FS_START_LOCATION + 3;
	superblock.blocks_start = superblock.inodes_start + (superblock.ninodes/ INODES_PER_BLOCK);

	superblock.block_map = create_bitmap(superblock.nblocks);
	superblock.inode_map = create_bitmap(superblock.ninodes);

	dbgprintf("[FS]: Creating Filesystem with size: %d (%d total)\n", superblock.nblocks*BLOCK_SIZE, superblock.size);
	dbgprintf("[FS]: With a total of %d inodes (%d blocks)\n", superblock.ninodes, superblock.ninodes / INODES_PER_BLOCK);
	dbgprintf("[FS]: And total of %d blocks\n", superblock.nblocks);
	dbgprintf("[FS]: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);

	inode_t root_inode = alloc_inode(&superblock, FS_DIRECTORY);
	root_dir = inode_get(root_inode, &superblock);
	root_dir->nlink++;

	superblock.root_inode = root_inode;

	struct directory_entry self = {
		.inode = root_inode,
		.name = "."
	};

	struct directory_entry back = {
		.inode = root_inode,
		.name = ".."
	};

	current_dir = root_dir;

	__inode_add_dir(&back, root_dir, &superblock);
	__inode_add_dir(&self, root_dir, &superblock);

	inode_t bin_dir = ext_create_directory("bin", root_inode);
	struct inode* bin_dir_inode = inode_get(bin_dir, &superblock);

	inode_t test_file_inode = alloc_inode(&superblock, FS_FILE);
	struct inode* test_file_disk_inode = inode_get(test_file_inode, &superblock);

	struct directory_entry hello_c = {
		.inode = test_file_inode,
		.name = "add.c"
	};

	char* test_file_text = "\n\
		\n\
		int add(int a, int b)\n\
		{\n\
			return a+b;\n\
		}\n\
		\n\
		int main()\n\
		{\n\
			int c;\n\
			\n\
			c = add(10, 13);\n\
			return c;\n\
		}\n";

	inode_write(test_file_text, strlen(test_file_text)+1, test_file_disk_inode, &superblock);

	/* Editor */
	inode_t home_inode = alloc_inode(&superblock, FS_FILE);
	struct inode* home_disk_inode = inode_get(home_inode, &superblock);

	struct directory_entry home = {
		.inode = home_inode,
		.name = "edit.o"
	};

	inode_write(apps_editor_edit_o, apps_editor_edit_o_len, home_disk_inode, &superblock);

	__inode_add_dir(&hello_c, root_dir, &superblock);
	__inode_add_dir(&home, bin_dir_inode, &superblock);


	root_dir->pos = 0;
}

int ext_create(char* name)
{
	if(strlen(name)+1 > FS_DIRECTORY_NAME_SIZE)
		return -FS_ERR_NAME_SIZE;

	inode_t inode_index = alloc_inode(&superblock, FS_FILE);
	if(inode_index == 0){
		return -FS_ERR_CREATE;
	}

	struct inode* inode = inode_get(inode_index, &superblock);

	struct directory_entry new = {
		.inode = inode->inode
	};
	memcpy(new.name, name, strlen(name));
	__inode_add_dir(&new, current_dir, &superblock);

	dbgprintf("[FS] Creating new file %s, inode: %d.\n", name, inode->inode);
	
	return 0;
} 

int ext_read(inode_t i, void* buf, int size)
{
	char* buffer = (char*)buf;
	struct inode* inode = inode_get(i, &superblock);
	if(inode == NULL)
		return -ERROR_NULL_POINTER;

	int ret = inode_read(buffer, size, inode, &superblock);
	
	dbgprintf("Read %d bytes from %d\n", ret, i);

	return ret;
}

/**
 * @brief Sets position attribute of inode
 * Cannot set posision past a files size.
 * @param i inode
 * @param pos position to set
 * @param opt UNUSED
 * @return int 
 */
int ext_seek(inode_t i, int pos, int opt)
{
	struct inode* inode = inode_get(i, &superblock);
	
	if(pos > inode->size)
		return -1;
	
	inode->pos = pos;
	return 0;
}


int ext_write(inode_t i, void* buf, int size)
{
	char* buffer = (char*) buf;
	struct inode* inode = inode_get(i, &superblock);
	inode->pos = 0; /* Should not set pos = 0*/
	
	dbgprintf("[FS] writing %d from inode %d\n", size, i);
	int ret = inode_write(buffer, size, inode, &superblock);
	
	return ret;
}

void ext_close(inode_t inode)
{
	struct inode* inode_disk = inode_get(inode, &superblock);
	if(inode_disk == NULL)
		return;

	dbgprintf("[FS] Closing inode %d\n", inode_disk->inode);
	inode_disk->nlink--;
}

inode_t ext_open_from_directory(char* name, inode_t i)
{
	struct directory_entry entry;
	struct inode* inode = inode_get(i, &superblock);
	inode->pos = 0;
	int size = 0;

	if(strlen(name)+1 > FS_DIRECTORY_NAME_SIZE)
		return -FS_ERR_NAME_SIZE;

	while (size <= inode->size)
	{
		int ret = inode_read((char*) &entry, sizeof(struct directory_entry), inode, &superblock);
		if(ret <= 0){
			return 0;
		}
		
		dbgprintf("[FS] Read %d / %d\n", size, inode->size);
		int mem_ret = memcmp((void*)entry.name, (void*)name, strlen(entry.name));
		if(mem_ret == 0)
			goto ext_open_done;
		size += ret;
	}

	return 0;

ext_open_done:
	dbgprintf("[FS] Opened file %s inode: %d\n", name, entry.inode);

	inode = inode_get(entry.inode, &superblock);
	if(inode == NULL)
		return -FS_ERR_FILE_MISSING;

	inode->nlink++;
	inode->pos = 0;

	
	return entry.inode;
}

inode_t ext_open(char* name, ext_flag_t flags)
{
	CHECK_DISK();

	/* TODO: check flags for read / write access */

	int ret = inode_from_path(name);
	if(ret <= 0 && flags & FS_FLAG_CREATE){
		ext_create(name);
		return inode_from_path(name);
	}

	struct inode* inode = inode_get(ret, &superblock);
	if(inode == NULL)
		return -FS_ERR_FILE_MISSING;

	inode->nlink++;
	inode->pos = 0;

	return ret;

}

int ext_size(inode_t i)
{
	struct inode* inode = inode_get(i, &superblock);
	return inode->size;
}

inode_t change_directory(char* path)
{
	inode_t ret = ext_open(path, 0);
	struct inode* inode = inode_get(ret, &superblock);
	if(inode->type != FS_DIRECTORY){
		return -FS_ERR_NOT_DIRECTORY;
	}

	dbgprintf("[FS] Changing directory to %s, inode: %d\n", path, inode->inode);

	/* This is this only for testing?? */
	current_dir = inode;

	return inode->inode;
}


/* returnrs 0 on success, < 0 on error */
inode_t ext_create_directory(char* name, inode_t current)
{
	if(strlen(name)+1 > FS_DIRECTORY_NAME_SIZE)
		return -FS_ERR_NAME_SIZE;

	inode_t inode_index = alloc_inode(&superblock, FS_DIRECTORY);
	if(inode_index == 0){
		return -FS_ERR_CREATE_INODE;
	}

	struct inode* inode = inode_get(inode_index, &superblock);
	struct inode* current_dir = inode_get(current, &superblock);
	if(inode == NULL)
		return -FS_ERR_INODE_MISSING;

	struct directory_entry self = {
		.inode = inode->inode,
		.name = "."
	};

	struct directory_entry back = {
		.inode = current_dir->inode,
		.name = ".."
	};

	struct directory_entry new = {
		.inode = inode->inode
	};
	memcpy(new.name, name, strlen(name));

	__inode_add_dir(&back, inode, &superblock);
	__inode_add_dir(&self, inode, &superblock);
	__inode_add_dir(&new, current_dir, &superblock);

	dbgprintf("[FS] Created directory %s with inode %d.\n", name, inode->inode);

	return inode_index;
}



void listdir()
{
	if(!disk_attached()) return;

	struct directory_entry entry;
	struct inode* current_dir = inode_get(ext_get_current_dir(), &superblock);
	int size = 0;

	twritef("Size  Date    Time    Name\n");
	current_dir->pos = 0;
	while (size < current_dir->size)
	{
		int ret = inode_read((char*) &entry, sizeof(struct directory_entry), current_dir, &superblock);
		struct inode* inode = inode_get(entry.inode, &superblock);
		struct time* time = &inode->time;
		twritef("%p %s %d, %d:%d - %s%s\n",
			inode->size,
			months[time->month],
			time->day, time->hour, time->minute,
			entry.name,
			inode->type == FS_DIRECTORY ? "/" : ""
		);
		size += ret;
	}
}