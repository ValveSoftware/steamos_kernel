#ifndef _BACKPORTS_LINUX_AF_ARP_H
#define _BACKPORTS_LINUX_AF_ARP_H 1

#include_next <linux/if_arp.h>

#ifndef ARPHRD_IEEE802154_MONITOR
#define ARPHRD_IEEE802154_MONITOR 805	/* IEEE 802.15.4 network monitor */
#endif

#endif /* _BACKPORTS_LINUX_AF_ARP_H */
