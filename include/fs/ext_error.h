#ifndef __FS_ERROR_H
#define __FS_ERROR_H

typedef enum ext_errors {
    FS_ERR_UNKNOWN = 1,
    FS_ERR_FILE_MISSING,
    FS_ERR_NOT_DIRECTORY,
    FS_ERR_NOT_FILE,
    FS_ERR_CREATE,
    FS_ERR_NAME_SIZE,
    FS_ERR_CREATE_INODE,
    FS_ERR_INODE_MISSING
} ext_error_t;

void ext_err(int error);

#endif /* __FS_ERROR_H */
