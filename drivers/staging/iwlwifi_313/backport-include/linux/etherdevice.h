#ifndef _BACKPORT_LINUX_ETHERDEVICE_H
#define _BACKPORT_LINUX_ETHERDEVICE_H
#include_next <linux/etherdevice.h>
#include <linux/version.h>
/*
 * newer kernels include this already and some
 * users rely on getting this indirectly
 */
#include <asm/unaligned.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
#define eth_hw_addr_random LINUX_BACKPORT(eth_hw_addr_random)
static inline void eth_hw_addr_random(struct net_device *dev)
{
#error eth_hw_addr_random() needs to be implemented for < 2.6.12
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
#define eth_hw_addr_random LINUX_BACKPORT(eth_hw_addr_random)
static inline void eth_hw_addr_random(struct net_device *dev)
{
	get_random_bytes(dev->dev_addr, ETH_ALEN);
	dev->dev_addr[0] &= 0xfe;       /* clear multicast bit */
	dev->dev_addr[0] |= 0x02;       /* set local assignment bit (IEEE802) */
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
/* So this is 2.6.31..2.6.35 */

/* Just have the flags present, they won't really mean anything though */
#define NET_ADDR_PERM		0	/* address is permanent (default) */
#define NET_ADDR_RANDOM		1	/* address is generated randomly */
#define NET_ADDR_STOLEN		2	/* address is stolen from other device */

#define eth_hw_addr_random LINUX_BACKPORT(eth_hw_addr_random)
static inline void eth_hw_addr_random(struct net_device *dev)
{
	random_ether_addr(dev->dev_addr);
}

#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
#define eth_hw_addr_random LINUX_BACKPORT(eth_hw_addr_random)
static inline void eth_hw_addr_random(struct net_device *dev)
{
	dev->addr_assign_type |= NET_ADDR_RANDOM;
	random_ether_addr(dev->dev_addr);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0)
#include <linux/random.h>
/**
 * eth_broadcast_addr - Assign broadcast address
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Assign the broadcast address to the given address array.
 */
#define eth_broadcast_addr LINUX_BACKPORT(eth_broadcast_addr)
static inline void eth_broadcast_addr(u8 *addr)
{
	memset(addr, 0xff, ETH_ALEN);
}

/**
 * eth_random_addr - Generate software assigned random Ethernet address
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Generate a random Ethernet address (MAC) that is not multicast
 * and has the local assigned bit set.
 */
#define eth_random_addr LINUX_BACKPORT(eth_random_addr)
static inline void eth_random_addr(u8 *addr)
{
	get_random_bytes(addr, ETH_ALEN);
	addr[0] &= 0xfe;        /* clear multicast bit */
	addr[0] |= 0x02;        /* set local assignment bit (IEEE802) */
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)

/* This backports:
 *
 * commit 6d57e9078e880a3dd232d579f42ac437a8f1ef7b
 * Author: Duan Jiong <djduanjiong@gmail.com>
 * Date:   Sat Sep 8 16:32:28 2012 +0000
 * 
 *     etherdevice: introduce help function eth_zero_addr() 
 */
#define eth_zero_addr LINUX_BACKPORT(eth_zero_addr)
static inline void eth_zero_addr(u8 *addr)
{
	memset(addr, 0x00, ETH_ALEN);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)
#define ether_addr_equal LINUX_BACKPORT(ether_addr_equal)
static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
	return !compare_ether_addr(addr1, addr2);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
#define alloc_etherdev_mqs(sizeof_priv, tx_q, rx_q) alloc_etherdev_mq(sizeof_priv, tx_q)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
/**
 * is_unicast_ether_addr - Determine if the Ethernet address is unicast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is a unicast address.
 */
#define is_unicast_ether_addr LINUX_BACKPORT(is_unicast_ether_addr)
static inline int is_unicast_ether_addr(const u8 *addr)
{
	return !is_multicast_ether_addr(addr);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
#define eth_prepare_mac_addr_change LINUX_BACKPORT(eth_prepare_mac_addr_change)
extern int eth_prepare_mac_addr_change(struct net_device *dev, void *p);

#define eth_commit_mac_addr_change LINUX_BACKPORT(eth_commit_mac_addr_change)
extern void eth_commit_mac_addr_change(struct net_device *dev, void *p);
#endif /* < 3.9 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
#define eth_mac_addr LINUX_BACKPORT(eth_mac_addr)
extern int eth_mac_addr(struct net_device *dev, void *p);
#define eth_change_mtu LINUX_BACKPORT(eth_change_mtu)
extern int eth_change_mtu(struct net_device *dev, int new_mtu);
#define eth_validate_addr LINUX_BACKPORT(eth_validate_addr)
extern int eth_validate_addr(struct net_device *dev);
#endif /* < 2.6.29 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#define netdev_hw_addr dev_mc_list
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
/**
 * eth_hw_addr_inherit - Copy dev_addr from another net_device
 * @dst: pointer to net_device to copy dev_addr to
 * @src: pointer to net_device to copy dev_addr from
 *
 * Copy the Ethernet address from one net_device to another along with
 * the address attributes (addr_assign_type).
 */
static inline void eth_hw_addr_inherit(struct net_device *dst,
				       struct net_device *src)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
	dst->addr_assign_type = src->addr_assign_type;
#endif
	memcpy(dst->dev_addr, src->dev_addr, ETH_ALEN);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0) */

#endif /* _BACKPORT_LINUX_ETHERDEVICE_H */
