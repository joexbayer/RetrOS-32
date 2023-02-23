#include <fs/path.h>
#include <fs/directory.h>
#include <fs/fs.h>
#include <util.h>

static int path_next_iter(char* path, char* next)
{
    int i = 0;
    while(path[i] != 0 && path[i] != '/')
        i++;

    if(path[i] == 0)
        return 1;

    memcpy(next, path, i);
    next[i+1] = 0;

    return 0;
}

inode_t inode_from_path_recursise(inode_t inode, char* path)
{

}

inode_t inode_from_path(char* path)
{
    char next_iter[FS_DIRECTORY_NAME_SIZE];

    switch (path[0])
    {
    case '/':
        if(path_next_iter(&path[1], next_iter)){

            return -1;
        }
        
        inode_t inode = open(next_iter);

        break;
    case '.':
        /* code */
        break;
    default:
        break;
    }    
}   