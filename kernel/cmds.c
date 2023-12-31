/**
 * @file cmds.c
 * @author Joe Bayer (joexbayer)
 * @brief All commands for the kernel
 * @version 0.1
 * @date 2023-12-24
 * 
 * Commands can be executed using exec_cmd(char *cmd)
 * Take in argc and argv, and can be run in parallel.
 * 
 * @copyright Copyright (c) 2023
 */

#include <kernel.h>
#include <ksyms.h>
#include <terminal.h>
#include <memory.h>
#include <fs/fs.h>

#define COMMAND(name, func) \
	int name\
		func\
	EXPORT_KSYMBOL(name);

/* Filesystem management */

/**
 * @brief Create a file or directory
 * Specified by second argument "file" or "dir"
 */
static int new(int argc, char* argv[])
{
    if(argc < 3) {
        twritef("Usage: new [file, dir] <name>\n");
        return 1;
    }

    if(strcmp(argv[1], "file") == 0) {

        int fd = fs_open(argv[2], FS_FILE_FLAG_CREATE);
        if(fd < 0) {
            twritef("Failed to create file %s\n", argv[2]);
            return 1;
        }

    } else if(strcmp(argv[1], "dir") == 0) {

        twritef("Currently unsupported.\n");
    
    } else {
        twritef("Usage: new <file, dir> <name>\n");
        return 1;
    }

    return 0;
}
EXPORT_KSYMBOL(new);

static int list(int argc, char* argv[])
{
    if(argc < 1) {
        twritef("Usage: list <path?>\n");
        return 1;
    }
    
    struct filesystem* fs = fs_get();
	if(fs == NULL){
		twritef("No filesystem mounted.\n");
		return;
	}

	if(fs->ops->list == NULL){
		twritef("Filesystem does not support listing\n");
		return;
	}

	if(argc == 1){
		fs->ops->list(fs, "/", NULL, 0);
		return;
	}

	fs->ops->list(fs, argv[1], NULL, 0);

    return 0;
}
EXPORT_KSYMBOL(list);

static int about(){

    struct memory_map* map = memory_map_get();

    /* neofetch style twritef */
    #define RETROS_ASCII_ART_INFO \
    "                  .----.      Kernel: RetrOS-32 0.0.1 alpha\n" \
    "      .---------. | == |      Build Date: " __DATE__ "\n" \
    "      |.-\"\"\"\"\"-.| |----|      Build Time: " __TIME__ "\n" \
    "      ||       || | == |      Memory: %d bytes\n" \
    "      ||RetrOS || |----|      Display: VGA\n" \
    "      |'-.....-'| |::::|                \n" \
    "      `\"\")---(\"\"` |___.|                \n" \
    "     /:::::::::::\" _  \"                \n" \
    "    /:::=======:::\\`\\`\\             \n" \
    "    `\"\"\"\"\"\"\"\"\"\"\"\"\"`  '-'                \n"

    twritef(RETROS_ASCII_ART_INFO, map->total);


    return 0;
}
EXPORT_KSYMBOL(about);

static int view(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: view <path>\n");
        return 1;
    }

    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return;
    }

    if(fs->ops->read == NULL){
            twritef("Filesystem does not support reading\n");
        return;
    }

    struct file* file = fs->ops->open(fs, argv[1], FS_FILE_FLAG_READ);
    if(file < 0) {
        twritef("Failed to open file %s\n", argv[1]);
        return 1;
    }

    if(file->size == 0) {
        twritef("File %s is empty\n", argv[1]);
        fs->ops->close(fs, file);
        return 1;
    }

    ubyte_t buf = kalloc(file->size);
    int len = fs->ops->read(fs, file, buf, file->size);
    if(len < 0) {
        twritef("Failed to read file %s\n", argv[1]);
        return 1;
    }

    twritef("%s\n", buf);

    fs->ops->close(fs, file);

    return 0;
}
EXPORT_KSYMBOL(view);

static int file(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: file <path>\n");
        return 1;
    }

    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return;
    }

    struct file* file = fs->ops->open(fs, argv[1], FS_FILE_FLAG_READ);
    if(file < 0) {
        twritef("Failed to open file %s\n", argv[1]);
        return 1;
    }

    twritef("File %s:\n", argv[1]);
    twritef("Nlinks: %d\n", file->nlinks);
    twritef("Directory: %d\n", file->directory);
    twritef("Size: %d\n", file->size);

    fs->ops->close(fs, file);
    return 0;
}
EXPORT_KSYMBOL(file);



/* Process management */

/* System management */

/* Network management */

/* Administration management */

/* package? */