#ifndef __BACKPORT_LINUX_IOPORT_H
#define __BACKPORT_LINUX_IOPORT_H
#include_next <linux/ioport.h>

#ifndef IORESOURCE_REG
#define IORESOURCE_REG		0x00000300
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
static inline resource_size_t resource_size(const struct resource *res)
{
	return res->end - res->start + 1;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27) */

#endif /* __BACKPORT_LINUX_IOPORT_H */
