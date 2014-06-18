#ifndef _COMPAT_LINUX_ATOMIC_H
#define _COMPAT_LINUX_ATOMIC_H 1

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
#include_next <linux/atomic.h>
#else
#include <asm/atomic.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)) */

#endif	/* _COMPAT_LINUX_ATOMIC_H */
