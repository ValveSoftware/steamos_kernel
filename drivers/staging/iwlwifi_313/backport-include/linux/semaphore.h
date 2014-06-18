#ifndef __BACKPORT_LINUX_SEMAPHORE_H
#define __BACKPORT_LINUX_SEMAPHORE_H

#include <linux/version.h>

#if  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include_next <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26) */

#endif /* __BACKPORT_LINUX_SEMAPHORE_H */
