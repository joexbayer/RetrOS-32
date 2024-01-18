#ifndef __ADMIN_H
#define __ADMIN_H

#include <kutils.h>

typedef enum {
    _ = 0,
    /* File System Permissions */
    ACCESS_FS_READ = 1 << 0,
    ACCESS_FS_WRITE = 1 << 1,
    ACCESS_FS_EXECUTE = 1 << 2,
    ACCESS_FS_RESERVED = 1 << 3,
    ACCESS_FS_RESERVED1 = 1 << 4,

    /* System Resource Access */
    ACCESS_NET = 1 << 5,
    ACCESS_HW = 1 << 6,
    ACCESS_RESERVED = 1 << 7,

    /* Process Management */
    CTRL_PROC_CREATE = 1 << 8,
    CTRL_PROC_TERMINATE = 1 << 9,
    CTRL_PROC_PRIORITIZE = 1 << 10,

    /* User Management */
    USER_CREATE = 1 << 11,
    USER_PWD_MANAGE = 1 << 12,
    USER_GROUP_MANAGE = 1 << 13,

    /* Security and Privacy */
    SEC_FW_CONFIG = 1 << 14,
    SEC_RESERVED = 1 << 15,
    SEC_RESERVED1 = 1 << 16,

    /* Software Management */
    SW_RESERVED = 1 << 17,
    SW_RESERVED1= 1 << 18,
    SW_RESERVED2 = 1 << 19,

    /* Device Management */
    DEV_PAIR = 1 << 20,
    DEV_CONFIG = 1 << 21,

    /* Special Administrative Permissions */
    ADMIN_FULL_ACCESS = 1 << 22,
    ADMIN_RESERVED = 1 << 23,
    ADMIN_RESERVED1 = 1 << 24
    
} permission_t;

#endif // !__ADMIN_H