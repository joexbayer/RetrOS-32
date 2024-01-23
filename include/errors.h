#ifndef EDD28A84_235C_41BD_B175_C5224DEECC55
#define EDD28A84_235C_41BD_B175_C5224DEECC55

#include <serial.h>
#include <kutils.h>

/* error code from errors.h */
typedef int error_t;

enum errors {
    ERROR_OK,
    ERROR_UNKNOWN,
    ERROR_NULL_POINTER,
    ERROR_INVALID_ARGUMENTS,
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
    ERROR_WINDOW_NOT_FOUND,
    ERROR_OPS_CORRUPTED,
    ERROR_OUT_OF_MEMORY,
    ERROR_ACCESS_DENIED,
};

char* error_get_string(error_t err);

#define RETURN_ON_ERR(exp) do{int ret; ret = exp; if(ret < 0){dbgprintf("WARNING: %s\n", error_get_string(ret)); return ret;}}while (0);
#define PANIC_ON_ERR(exp) do{int ret; ret = exp; if(ret < 0){kernel_panic(error_get_string(ret));}}while (0);
#define ERR_ON_NULL(exp) do{if(exp == NULL){dbgprintf("WARNING: %s\n", error_get_string(-ERROR_NULL_POINTER)); return -ERROR_NULL_POINTER;}}while (0);
#define ERR_ON_NULL_PTR(exp) do{if(exp == NULL){dbgprintf("WARNING: %s\n", error_get_string(-ERROR_NULL_POINTER)); return NULL;}}while (0);

#endif /* EDD28A84_235C_41BD_B175_C5224DEECC55 */
