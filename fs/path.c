#include <fs/path.h>
#include <fs/directory.h>
#include <fs/fs.h>
#include <util.h>

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
            return fs_open_from_directory(next_iter, inode);
        }
        
        /* not last item, find next */
        inode_t next = fs_open_from_directory(next_iter, inode);
        if(next <= 0)
            return next;
        
        /* continue search in next */
        return inode_from_path_recursise(next, &path[ret]);
        
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
    inode_t dir = fs_get_current_dir();

    switch (path[0])
    {
    case '/':
        index = 1;
        dir = fs_get_root();
        break;
    case '.':
        index = 2;
        break;
    default:
        break;
    }

    int ret = path_next_iter(&path[index], next_iter);
    if(ret < 1){
        return fs_open_from_directory(next_iter, dir);
    }

    inode_t next = fs_open_from_directory(next_iter, dir);
    if(next <= 0)
        return next;
    
    inode_t final = inode_from_path_recursise(next, &path[ret]);
    return final;
}   