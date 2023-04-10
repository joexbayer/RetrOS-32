#ifndef KTHREADS_H
#define KTHREADS_H

#include <stdint.h>
#include <errors.h>

/* kthreads functions: FIXME */
void shell_main();
void networking_main();
void dhcpd();
void error_main();
void tcpd();
void image_viewer();

error_t start(char* name);
error_t register_kthread(void (*f)(), char* name);

#endif /* KTHREADS_H */
