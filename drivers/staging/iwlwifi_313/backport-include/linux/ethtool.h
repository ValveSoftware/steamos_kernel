#ifndef __BACKPORT_LINUX_ETHTOOL_H
#define __BACKPORT_LINUX_ETHTOOL_H
#include_next <linux/ethtool.h>
#include <linux/version.h>

#ifndef SPEED_UNKNOWN
#define SPEED_UNKNOWN  -1
#endif /* SPEED_UNKNOWN */

#ifndef DUPLEX_UNKNOWN
#define DUPLEX_UNKNOWN 0xff
#endif /* DUPLEX_UNKNOWN */

#ifndef ETHTOOL_FWVERS_LEN
#define ETHTOOL_FWVERS_LEN 32
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
#define SUPPORTED_Backplane            (1 << 16)
#define SUPPORTED_1000baseKX_Full      (1 << 17)
#define SUPPORTED_10000baseKX4_Full    (1 << 18)
#define SUPPORTED_10000baseKR_Full     (1 << 19)
#define SUPPORTED_10000baseR_FEC       (1 << 20)

#define ADVERTISED_Backplane           (1 << 16)
#define ADVERTISED_1000baseKX_Full     (1 << 17)
#define ADVERTISED_10000baseKX4_Full   (1 << 18)
#define ADVERTISED_10000baseKR_Full    (1 << 19)
#define ADVERTISED_10000baseR_FEC      (1 << 20)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
static inline void ethtool_cmd_speed_set(struct ethtool_cmd *ep,
					 __u32 speed)
{
	ep->speed = (__u16)speed;
}

static inline __u32 ethtool_cmd_speed(const struct ethtool_cmd *ep)
{
	return ep->speed;
}
#endif

#endif /* __BACKPORT_LINUX_ETHTOOL_H */
