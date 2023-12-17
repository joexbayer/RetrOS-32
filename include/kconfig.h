#ifndef __KCONFIG_H__
#define __KCONFIG_H__

#include <kutils.h>

/**
 * @brief Basic config constants for the kernel
 */

#define PCB_MAX_PROCESSES 32
#define PCB_MAX_THREADS 32

/**
 * @brief Debug defines
 * 
 */
#define KDEBUG
#define KDEBUG_SERIAL
#define KDEBUG_WARNINGS

/* specifics */
//#define KDEBUG_INTERRUPTS
//#define KDEBUG_MEMORY
#define KDEBUG_SCHEDULER
#define KDEBUG_SYSCALLS
#define KDEBUG_THREADS
//#define KDEBUG_SYNC
//#define KDEBUG_WORKER

/* net */
//#define KDEBUG_NET_ARP
#define KDEBUG_NET_ICMP
//#define KDEBUG_NET_IP
#define KDEBUG_NET_TCP
#define KDEBUG_NET_UDP

/* devices */

/* fs */


#define KERNEL_PANIC_ON_PAGE_FAULT




#endif /* !__KCONFIG_H__ */
