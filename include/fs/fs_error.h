#ifndef __FS_ERROR_H
#define __FS_ERROR_H

enum fs_errors {
    FS_ERR_UNKOWN = -1,
    FS_ERR_FILE_MISSING = -2,
    FS_ERR_NOT_DIRECTORY = -3,
    FS_ERR_NOT_FILE = -4
};

void fs_err(int error);

#endif /* __FS_ERROR_H */
