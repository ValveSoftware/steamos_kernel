#ifndef __BACKPORT_IN_H
#define __BACKPORT_IN_H
#include_next <linux/in.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static inline int proto_ports_offset(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:	/* SPI */
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
		return 0;
	case IPPROTO_AH:	/* SPI */
		return 4;
	default:
		return -EINVAL;
	}
}
#endif

#endif /* __BACKPORT_IN_H */
