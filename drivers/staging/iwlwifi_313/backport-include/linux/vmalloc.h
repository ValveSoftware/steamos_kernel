#ifndef __BACKPORT_LINUX_VMALLOC_H
#define __BACKPORT_LINUX_VMALLOC_H
#include_next <linux/vmalloc.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#define vzalloc LINUX_BACKPORT(vzalloc)
extern void *vzalloc(unsigned long size);
#endif

#endif /* __BACKPORT_LINUX_VMALLOC_H */
