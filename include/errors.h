#ifndef EDD28A84_235C_41BD_B175_C5224DEECC55
#define EDD28A84_235C_41BD_B175_C5224DEECC55

#include <serial.h>

typedef int error_t;

enum errors {
    ERROR_OK,
    ERROR_UNKNOWN,
    ERROR_WORK_QUEUE_FULL,
    ERROR_ALLOC,
    ERROR_INDEX,
    ERROR_FILE_NOT_FOUND,
    ERROR_PCB_FULL,
    ERROR_KTHREAD_CREATE,
    ERROR_KTHREAD_START,
    ERROR_INVALID_SOCKET,
    ERROR_INVALID_SOCKET_TYPE,
    ERROR_MSS_SIZE,
    ERROR_RBUFFER_FULL,
    ERROR_RBUFFER_EMPTY,
    ERROR_PCB_NULL,
    ERROR_PCB_QUEUE_NULL,
    ERROR_PCB_QUEUE_CREATE,
    ERROR_PCB_QUEUE_EMPTY,
    ERROR_SCHED_EXISTS,
    ERROR_SCHED_INVALID,
};

char* error_get_string(error_t err);

#define RETURN_ON_ERR(exp) do{int ret; ret = exp; if(ret != 0){dbgprintf("WARNING: %s\n", error_get_string(ret)); return ret;}}while (0);

#endif /* EDD28A84_235C_41BD_B175_C5224DEECC55 */
