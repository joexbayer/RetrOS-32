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

inode_t inode_from_path_recursise(inode_t inode, char* path)
{

}

inode_t inode_from_path(char* path)
{
    char next_iter[20];

    switch (path[0])
    {
    case '/':;
        int ret = path_next_iter(&path[1], next_iter);
        if(ret < 1){
            /* Found last item in path */
            return ret;
        }
        
        /* not last item, find next */
        inode_from_path(&path[ret]);
        
        break;
    case '.':
        /* code */
        break;
    default:
        break;
    }    
}   