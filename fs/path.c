#include <fs/path.h>
#include <fs/directory.h>
#include <fs/ext.h>
#include <util.h>
#include <serial.h>

static int path_next_iter(char* path, char* next)
{
    int i = 0;
    while(path[i] != 0 && path[i] != '/')
        i++;
    
    memcpy(next, path, i);
    next[i] = 0;
    
    if(path[i] == 0)
        return -1;

    return i+1;
}

static inode_t inode_from_path_recursise(inode_t inode, char* path)
{
    char next_iter[20];

    switch (path[0])
    {
    case '/':;
        int ret = path_next_iter(&path[1], next_iter);
        if(ret < 1){
            /* Found last item in path */
            return ext_open_from_directory(next_iter, inode);
        }
        
        /* not last item, find next */
        inode_t next = ext_open_from_directory(next_iter, inode);
        if(next <= 0)
            return next;

        /* continue search in next TODO: close next */
        inode_t result = inode_from_path_recursise(next, &path[ret]);
        ext_close(next);
        return result;
        
        break;
    default:
        break;
    }

    return -1;
}

inode_t inode_from_path(char* path)
{
    char next_iter[20];
    int index = 0;

#ifdef __FS_TEST
    inode_t dir = 1;
#else
    inode_t dir = ext_get_current_dir();
#endif

    dbgprintf("Looking for %s (%d)\n", path, dir);

    switch (path[0])
    {
    case '/':
        index = 1;
        dir = ext_get_root();
        break;
    case '.':
        if(path[1] == '/')
            index = 2;  
        break;
    default:
        break;
    }

    int ret = path_next_iter(&path[index], next_iter);
    if(ret < 1){
        return ext_open_from_directory(next_iter, dir);
    }

    inode_t next = ext_open_from_directory(next_iter, dir);
    if(next <= 0)
        return next;

    inode_t final = inode_from_path_recursise(next, &path[ret]);
    ext_close(next);
    return final;
}   