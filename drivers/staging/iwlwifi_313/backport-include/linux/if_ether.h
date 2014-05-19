#ifndef __BACKPORT_IF_ETHER_H
#define __BACKPORT_IF_ETHER_H
#include_next <linux/if_ether.h>

/*
 * backport of:
 * commit e5c5d22e8dcf7c2d430336cbf8e180bd38e8daf1
 * Author: Simon Horman <horms@verge.net.au>
 * Date:   Thu Mar 28 13:38:25 2013 +0900
 * 
 *     net: add ETH_P_802_3_MIN
 */
#ifndef ETH_P_802_3_MIN
#define ETH_P_802_3_MIN 0x0600
#endif

#ifndef ETH_P_TDLS
#define ETH_P_TDLS	0x890D          /* TDLS */
#endif

#ifndef ETH_P_LINK_CTL
#define ETH_P_LINK_CTL	0x886c
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#define mac_pton LINUX_BACKPORT(mac_pton)
int mac_pton(const char *s, u8 *mac);
#endif

#ifndef ETH_P_PAE
#define ETH_P_PAE 0x888E      /* Port Access Entity (IEEE 802.1X) */
#endif

#endif /* __BACKPORT_IF_ETHER_H */
