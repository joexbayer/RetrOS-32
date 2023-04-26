#ifndef KTHREADS_H
#define KTHREADS_H

#include <stdint.h>
#include <kutils.h>
#include <errors.h>

/* kthreads functions: FIXME */
void networking_main();
void dhcpd();
void error_main();
void tcpd();
void image_viewer();

error_t start(char* name);
error_t register_kthread(void (*f)(), char* name);

/**
 * @brief EXPORT_KTHREAD
 * Exports function to the kernels kthread table
 * @param func function to export as thread
 */
#define EXPORT_KTHREAD(func) \
    static void __register_kthread_##func() { register_kthread(func, #func); } \
    EXPORT_KCTOR(__register_kthread_##func)

#endif /* KTHREADS_H */
