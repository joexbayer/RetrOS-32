#include <fs/fs.h>
#include <fs/fs_error.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/directory.h>
#include <memory.h>
#include <diskdev.h>
#include <terminal.h>
#include <bitmap.h>
#include <serial.h>

#include <util.h>
#include <rtc.h>

static struct superblock superblock;
static struct inode* root_dir;
static struct inode* current_dir;

static int FS_START_LOCATION = 0;
static int FS_INODE_BMAP_LOCATION = 0;
static int FS_BLOCK_BMAP_LOCATION = 0;

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

int init_fs()
{
	FS_START_LOCATION = (kernel_size/512)+2;
	FS_INODE_BMAP_LOCATION = FS_START_LOCATION+1;
	FS_BLOCK_BMAP_LOCATION = FS_INODE_BMAP_LOCATION+1;

	/* Read superblock and check magic. */
	read_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
	if(superblock.magic != MAGIC){
		mkfs();
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

int fs_get_size()
{
	return superblock.size;
}

int fs_get_inodes()
{
	return superblock.ninodes;
}

int fs_get_blocks()
{
	return superblock.nblocks;
}

void __superblock_sync()
{
	write_block_offset((char*) &superblock, sizeof(struct superblock), 0, FS_START_LOCATION);
	write_block_offset((char*) superblock.inode_map, get_bitmap_size(superblock.ninodes), 0, FS_INODE_BMAP_LOCATION);
	//write_block_offset((char*) superblock.block_map, get_bitmap_size(superblock.nblocks), 0, FS_BLOCK_BMAP_LOCATION);

	dbgprintf("[FS] Superblock... (DONE)\n");
}

void sync()
{
	/*TODO: Causes pagefault! */
	dbgprintf("[FS] Synchronizing filesystem.\n");
	__superblock_sync();
	inodes_sync(&superblock);
	dbgprintf("[FS] %d inodes... (DONE)\n", superblock.ninodes);
}

static inline void __inode_add_dir(struct directory_entry* entry, struct inode* inode, struct superblock* sb)
{
	inode->pos = inode->size;
	inode_write((char*) entry, sizeof(struct directory_entry), inode, sb);
}

void mkfs()
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
	dbgprintf("[FS]: And total of %d block\n", superblock.nblocks);
	dbgprintf("[FS]: Max file size: %d bytes\n", NDIRECT*BLOCK_SIZE);

	inode_t root_inode = alloc_inode(&superblock, FS_DIRECTORY);
	struct inode* root = inode_get(root_inode, &superblock);
	root->nlink++;

	superblock.root_inode = root_inode;

	struct directory_entry self = {
		.inode = root_inode,
		.name = "."
	};

	struct directory_entry back = {
		.inode = root_inode,
		.name = ".."
	};

	inode_t home_inode = alloc_inode(&superblock, FS_FILE);
	struct inode* home_disk_inode = inode_get(home_inode, &superblock);

	struct directory_entry home = {
		.inode = home_inode,
		.name = "home"
	};

	char* home_text = "Home is where the heart is.";
	inode_write(home_text, strlen(home_text)+1, home_disk_inode, &superblock);

	root_dir = root;
	current_dir = root;

	__inode_add_dir(&back, current_dir, &superblock);
	__inode_add_dir(&self, current_dir, &superblock);
	__inode_add_dir(&home, current_dir, &superblock);

	current_dir->pos = 0;
}

int fs_create(char* name)
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

int fs_read(char* buf, inode_t i)
{
	struct inode* inode = inode_get(i, &superblock);
	inode->pos = 0; /* Should not set pos = 0*/
	
	int ret = inode_read(buf, inode->size, inode, &superblock);
	return ret;
}

int fs_write(void* buf, int size, inode_t i)
{
	char* buffer = (char*) buf;
	struct inode* inode = inode_get(i, &superblock);
	inode->pos = 0; /* Should not set pos = 0*/
	
	dbgprintf("[FS] writing %d from inode %d\n", size, i);
	int ret = inode_write(buffer, size, inode, &superblock);
	return ret;
}

void fs_close(inode_t inode)
{
	struct inode* inode_disk = inode_get(inode, &superblock);
	inode_disk->nlink--;
}

inode_t fs_open(char* name)
{
	struct directory_entry entry;
	current_dir->pos = 0;
	int size = 0;

	if(strlen(name)+1 > FS_DIRECTORY_NAME_SIZE)
		return -FS_ERR_NAME_SIZE;

	while (size < current_dir->size)
	{
		int ret = inode_read((char*) &entry, sizeof(struct directory_entry), current_dir, &superblock);
		int mem_ret = memcmp((void*)entry.name, (void*)name, strlen(entry.name));
		if(mem_ret == 0)
			goto fs_open_done;
		size += ret;
	}

	return 0;

fs_open_done:
	dbgprintf("[FS] Opened file %s inode: %d\n", name, entry.inode);

	struct inode* inode = inode_get(entry.inode, &superblock);
	if(inode == NULL)
		return -FS_ERR_FILE_MISSING;

	inode->nlink++;
	inode->pos = 0;

	
	return entry.inode;
}

int fs_size(inode_t i)
{
	struct inode* inode = inode_get(i, &superblock);
	return inode->size;
}

int chdir(char* path)
{
	inode_t ret = fs_open(path);
	struct inode* inode = inode_get(ret, &superblock);
	if(inode->type != FS_DIRECTORY){
		return -FS_ERR_NOT_DIRECTORY;
	}

	dbgprintf("[FS] Changing directory to %s, inode: %d\n", path, inode->inode);

	current_dir = inode;

	return 0;
}

int fs_mkdir(char* name)
{
	if(strlen(name)+1 > FS_DIRECTORY_NAME_SIZE)
		return -FS_ERR_NAME_SIZE;

	inode_t inode_index = alloc_inode(&superblock, FS_DIRECTORY);
	if(inode_index == 0){
		return -FS_ERR_CREATE_INODE;
	}

	struct inode* inode = inode_get(inode_index, &superblock);
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

	return 0;
}

void ls(char* path)
{
	struct directory_entry entry;
	int size = 0;

	current_dir->pos = 0;
	while (size < current_dir->size)
	{
		int ret = inode_read((char*) &entry, sizeof(struct directory_entry), current_dir, &superblock);
		struct inode* inode = inode_get(entry.inode, &superblock);
		struct time* time = &inode->time;
		dbgprintf("%d\n", time->month);
		twritef("%x %s %d, %d:%d - %s%s\n",
			inode->size,
			months[time->month],
			time->day, time->hour, time->minute,
			entry.name,
			inode->type == FS_DIRECTORY ? "/" : ""
		);
		size += ret;
	}
}