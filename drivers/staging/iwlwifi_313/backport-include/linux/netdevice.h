#ifndef __BACKPORT_NETDEVICE_H
#define __BACKPORT_NETDEVICE_H
#include_next <linux/netdevice.h>
#include <linux/netdev_features.h>
#include <linux/version.h>

/*
 * This is declared implicitly in newer kernels by netdevice.h using
 * this pointer in struct net_device, but declare it here anyway so
 * pointers to it are accepted as function arguments without warning.
 */
struct inet6_dev;

/* older kernels don't include this here, we need it */
#include <linux/ethtool.h>
#include <linux/rculist.h>
/*
 * new kernels include <net/netprio_cgroup.h> which
 * has this ... and some drivers rely on it :-(
 */
#include <linux/hardirq.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#define dev_change_net_namespace(a, b, c) (-EOPNOTSUPP)

static inline void SET_NETDEV_DEVTYPE(struct net_device *dev, void *type)
{
	/* nothing */
}

typedef int netdev_tx_t;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
/*
 * Older kernels do not have struct net_device_ops but what we can
 * do is just define the data structure and use a caller to let us
 * set the data structure's routines onto the old netdev, essentially
 * doing it the old way. This avoids huge deltas on our backports.
 */
#define HAVE_NET_DEVICE_OPS
struct net_device_ops {
	int			(*ndo_init)(struct net_device *dev);
	void			(*ndo_uninit)(struct net_device *dev);
	int			(*ndo_open)(struct net_device *dev);
	int			(*ndo_stop)(struct net_device *dev);
	netdev_tx_t		(*ndo_start_xmit) (struct sk_buff *skb,
						   struct net_device *dev);
	u16			(*ndo_select_queue)(struct net_device *dev,
						    struct sk_buff *skb);
	void			(*ndo_change_rx_flags)(struct net_device *dev,
						       int flags);
	void			(*ndo_set_rx_mode)(struct net_device *dev);
	void			(*ndo_set_multicast_list)(struct net_device *dev);
	int			(*ndo_set_mac_address)(struct net_device *dev,
						       void *addr);
	int			(*ndo_validate_addr)(struct net_device *dev);
	int			(*ndo_do_ioctl)(struct net_device *dev,
					        struct ifreq *ifr, int cmd);
	int			(*ndo_set_config)(struct net_device *dev,
					          struct ifmap *map);
	int			(*ndo_change_mtu)(struct net_device *dev,
						  int new_mtu);
	int			(*ndo_neigh_setup)(struct net_device *dev,
						   struct neigh_parms *);
	void			(*ndo_tx_timeout) (struct net_device *dev);

	struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);

	void			(*ndo_vlan_rx_register)(struct net_device *dev,
						        struct vlan_group *grp);
	void			(*ndo_vlan_rx_add_vid)(struct net_device *dev,
						       unsigned short vid);
	void			(*ndo_vlan_rx_kill_vid)(struct net_device *dev,
						        unsigned short vid);
#ifdef CONFIG_NET_POLL_CONTROLLER
	void                    (*ndo_poll_controller)(struct net_device *dev);
#endif
	int			(*ndo_set_vf_mac)(struct net_device *dev,
						  int queue, u8 *mac);
	int			(*ndo_set_vf_vlan)(struct net_device *dev,
						   int queue, u16 vlan, u8 qos);
	int			(*ndo_set_vf_tx_rate)(struct net_device *dev,
						      int vf, int rate);
#if defined(CONFIG_FCOE) || defined(CONFIG_FCOE_MODULE)
	int			(*ndo_fcoe_enable)(struct net_device *dev);
	int			(*ndo_fcoe_disable)(struct net_device *dev);
	int			(*ndo_fcoe_ddp_setup)(struct net_device *dev,
						      u16 xid,
						      struct scatterlist *sgl,
						      unsigned int sgc);
	int			(*ndo_fcoe_ddp_done)(struct net_device *dev,
						     u16 xid);
#define NETDEV_FCOE_WWNN 0
#define NETDEV_FCOE_WWPN 1
	int			(*ndo_fcoe_get_wwn)(struct net_device *dev,
						    u64 *wwn, int type);
#endif
};

static inline struct net_device_stats *dev_get_stats(struct net_device *dev)
{
	return dev->get_stats(dev);
}

#define init_dummy_netdev LINUX_BACKPORT(init_dummy_netdev)
extern int init_dummy_netdev(struct net_device *dev);

#define napi_gro_receive(napi, skb) netif_receive_skb(skb)
#endif /* < 2.6.29 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,8)
#define netdev_set_default_ethtool_ops LINUX_BACKPORT(netdev_set_default_ethtool_ops)
extern void netdev_set_default_ethtool_ops(struct net_device *dev,
					   const struct ethtool_ops *ops);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
#define netdev_attach_ops LINUX_BACKPORT(netdev_attach_ops)
void netdev_attach_ops(struct net_device *dev,
		       const struct net_device_ops *ops);

static inline int ndo_do_ioctl(struct net_device *dev,
			       struct ifreq *ifr,
			       int cmd)
{
	if (dev->do_ioctl)
		return dev->do_ioctl(dev, ifr, cmd);
	return -EOPNOTSUPP;
}
#else
/* XXX: this can probably just go upstream ! */
static inline void netdev_attach_ops(struct net_device *dev,
		       const struct net_device_ops *ops)
{
	dev->netdev_ops = ops;
}

/* XXX: this can probably just go upstream! */
static inline int ndo_do_ioctl(struct net_device *dev,
			       struct ifreq *ifr,
			       int cmd)
{
	if (dev->netdev_ops && dev->netdev_ops->ndo_do_ioctl)
		return dev->netdev_ops->ndo_do_ioctl(dev, ifr, cmd);
	return -EOPNOTSUPP;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
/*
 * BQL was added as of v3.3 but some Linux distributions
 * have backported BQL to their v3.2 kernels or older. To
 * address this we assume that they also enabled CONFIG_BQL
 * and test for that here and simply avoid adding the static
 * inlines if it was defined
 */
#ifndef CONFIG_BQL
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#define netdev_tx_sent_queue LINUX_BACKPORT(netdev_tx_sent_queue)
static inline void netdev_tx_sent_queue(struct netdev_queue *dev_queue,
					unsigned int bytes)
{
}
#endif

#define netdev_sent_queue LINUX_BACKPORT(netdev_sent_queue)
static inline void netdev_sent_queue(struct net_device *dev, unsigned int bytes)
{
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#define netdev_tx_completed_queue LINUX_BACKPORT(netdev_tx_completed_queue)
static inline void netdev_tx_completed_queue(struct netdev_queue *dev_queue,
					     unsigned pkts, unsigned bytes)
{
}
#endif

#define netdev_completed_queue LINUX_BACKPORT(netdev_completed_queue)
static inline void netdev_completed_queue(struct net_device *dev,
					  unsigned pkts, unsigned bytes)
{
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#define netdev_tx_reset_queue LINUX_BACKPORT(netdev_tx_reset_queue)
static inline void netdev_tx_reset_queue(struct netdev_queue *q)
{
}
#endif

#define netdev_reset_queue LINUX_BACKPORT(netdev_reset_queue)
static inline void netdev_reset_queue(struct net_device *dev_queue)
{
}
#endif /* CONFIG_BQL */
#endif /* < 3.3 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
/*
 * since commit 1c5cae815d19ffe02bdfda1260949ef2b1806171
 * "net: call dev_alloc_name from register_netdevice" dev_alloc_name is
 * called automatically. This is not implemented in older kernel
 * versions so it will result in device wrong names.
 */
static inline int register_netdevice_name(struct net_device *dev)
{
	int err;

	if (strchr(dev->name, '%')) {
		err = dev_alloc_name(dev, dev->name);
		if (err < 0)
			return err;
	}

	return register_netdevice(dev);
}

#define register_netdevice(dev) register_netdevice_name(dev)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
#define alloc_netdev_mqs(sizeof_priv, name, setup, txqs, rxqs) \
	alloc_netdev_mq(sizeof_priv, name, setup, \
			max_t(unsigned int, txqs, rxqs))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#define netdev_refcnt_read(a) atomic_read(&a->refcnt)

#define net_ns_type_operations LINUX_BACKPORT(net_ns_type_operations)
extern struct kobj_ns_type_operations net_ns_type_operations;

#if (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,4))
#ifdef CONFIG_RPS
extern int netif_set_real_num_rx_queues(struct net_device *dev,
					unsigned int rxq);
#else
static inline int netif_set_real_num_rx_queues(struct net_device *dev,
					       unsigned int rxq)
{
	return 0;
}
#endif
#endif
#endif /* < 2.6.37 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
/*
 * etherdevice.h requires netdev_hw_addr to not have been redefined,
 * so while generally we shouldn't/wouldn't include unrelated header
 * files here it's unavoidable. However, if we got included through
 * it, then we let it sort out the netdev_hw_addr define so that it
 * still gets the correct one later ...
 */
#include <linux/etherdevice.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#define netif_set_real_num_tx_queues LINUX_BACKPORT(netif_set_real_num_tx_queues)
extern int netif_set_real_num_tx_queues(struct net_device *dev,
					unsigned int txq);
#define mc_addr(ha) (ha)->dmi_addr
#else
#define mc_addr(ha) (ha)->addr
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#define netdev_mc_count(dev) ((dev)->mc_count)
#define netdev_mc_empty(dev) (netdev_mc_count(dev) == 0)

/* mask netdev_for_each_mc_addr as RHEL6 backports this */
#ifndef netdev_for_each_mc_addr
#define netdev_for_each_mc_addr(mclist, dev) \
	for (mclist = dev->mc_list; mclist; mclist = mclist->next)
#endif

#ifndef netdev_name
#define netdev_name(__dev) \
	((__dev->reg_state != NETREG_REGISTERED) ? \
		"(unregistered net_device)" : __dev->name)
#endif

#define netdev_printk(level, netdev, format, args...)		\
	dev_printk(level, (netdev)->dev.parent,			\
		   "%s: " format,				\
		   netdev_name(netdev), ##args)

#define netdev_emerg(dev, format, args...)			\
	netdev_printk(KERN_EMERG, dev, format, ##args)
#define netdev_alert(dev, format, args...)			\
	netdev_printk(KERN_ALERT, dev, format, ##args)
#define netdev_crit(dev, format, args...)			\
	netdev_printk(KERN_CRIT, dev, format, ##args)
#define netdev_err(dev, format, args...)			\
	netdev_printk(KERN_ERR, dev, format, ##args)
#define netdev_warn(dev, format, args...)			\
	netdev_printk(KERN_WARNING, dev, format, ##args)
#define netdev_notice(dev, format, args...)			\
	netdev_printk(KERN_NOTICE, dev, format, ##args)
#define netdev_info(dev, format, args...)			\
	netdev_printk(KERN_INFO, dev, format, ##args)

/* mask netdev_dbg as RHEL6 backports this */
#if !defined(netdev_dbg)

#if defined(DEBUG)
#define netdev_dbg(__dev, format, args...)			\
	netdev_printk(KERN_DEBUG, __dev, format, ##args)
#elif defined(CONFIG_DYNAMIC_DEBUG)
#define netdev_dbg(__dev, format, args...)			\
do {								\
	dynamic_dev_dbg((__dev)->dev.parent, "%s: " format,	\
			netdev_name(__dev), ##args);		\
} while (0)
#else
#define netdev_dbg(__dev, format, args...)			\
({								\
	if (0)							\
		netdev_printk(KERN_DEBUG, __dev, format, ##args); \
	0;							\
})
#endif

#endif

/* mask netdev_vdbg as RHEL6 backports this */
#if !defined(netdev_dbg)

#if defined(VERBOSE_DEBUG)
#define netdev_vdbg	netdev_dbg
#else

#define netdev_vdbg(dev, format, args...)			\
({								\
	if (0)							\
		netdev_printk(KERN_DEBUG, dev, format, ##args);	\
	0;							\
})
#endif

#endif

/*
 * netdev_WARN() acts like dev_printk(), but with the key difference
 * of using a WARN/WARN_ON to get the message out, including the
 * file/line information and a backtrace.
 */
#define netdev_WARN(dev, format, args...)			\
	WARN(1, "netdevice: %s\n" format, netdev_name(dev), ##args);

/* netif printk helpers, similar to netdev_printk */

#define netif_printk(priv, type, level, dev, fmt, args...)	\
do {					  			\
	if (netif_msg_##type(priv))				\
		netdev_printk(level, (dev), fmt, ##args);	\
} while (0)

#define netif_emerg(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_EMERG, dev, fmt, ##args)
#define netif_alert(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_ALERT, dev, fmt, ##args)
#define netif_crit(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_CRIT, dev, fmt, ##args)
#define netif_err(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_ERR, dev, fmt, ##args)
#define netif_warn(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_WARNING, dev, fmt, ##args)
#define netif_notice(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_NOTICE, dev, fmt, ##args)
#define netif_info(priv, type, dev, fmt, args...)		\
	netif_printk(priv, type, KERN_INFO, (dev), fmt, ##args)

/* mask netif_dbg as RHEL6 backports this */
#if !defined(netif_dbg)

#if defined(DEBUG)
#define netif_dbg(priv, type, dev, format, args...)		\
	netif_printk(priv, type, KERN_DEBUG, dev, format, ##args)
#elif defined(CONFIG_DYNAMIC_DEBUG)
#define netif_dbg(priv, type, netdev, format, args...)		\
do {								\
	if (netif_msg_##type(priv))				\
		dynamic_dev_dbg((netdev)->dev.parent,		\
				"%s: " format,			\
				netdev_name(netdev), ##args);	\
} while (0)
#else
#define netif_dbg(priv, type, dev, format, args...)			\
({									\
	if (0)								\
		netif_printk(priv, type, KERN_DEBUG, dev, format, ##args); \
	0;								\
})
#endif

#endif

/* mask netif_vdbg as RHEL6 backports this */
#if !defined(netif_vdbg)

#if defined(VERBOSE_DEBUG)
#define netif_vdbg	netdev_dbg
#else
#define netif_vdbg(priv, type, dev, format, args...)		\
({								\
	if (0)							\
		netif_printk(KERN_DEBUG, dev, format, ##args);	\
	0;							\
})
#endif
#endif

#endif /* < 2.6.34 */

/* mask NETDEV_POST_INIT as RHEL6 backports this */
/* this will never happen on older kernels */
#ifndef NETDEV_POST_INIT
#define NETDEV_POST_INIT 0xffff
#endif

#ifndef NETDEV_PRE_UP
#define NETDEV_PRE_UP		0x000D
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
/*
 * On older kernels we do not have net_device Multi Queue support, but
 * since we no longer use MQ on mac80211 we can simply use the 0 queue.
 * Note that if other fullmac drivers make use of this they then need
 * to be backported somehow or deal with just 1 queue from MQ.
 */
static inline void netif_tx_wake_all_queues(struct net_device *dev)
{
	netif_wake_queue(dev);
}
static inline void netif_tx_start_all_queues(struct net_device *dev)
{
	netif_start_queue(dev);
}
static inline void netif_tx_stop_all_queues(struct net_device *dev)
{
	netif_stop_queue(dev);
}

/*
 * The net_device has a spin_lock on newer kernels, on older kernels we're out of luck
 */
#define netif_addr_lock_bh(dev)
#define netif_addr_unlock_bh(dev)

#define netif_wake_subqueue netif_start_subqueue
#endif /* < 2.6.27 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
static inline
struct net *dev_net(const struct net_device *dev)
{
#ifdef CONFIG_NET_NS
	/*
	 * compat-wirelss backport note:
	 * For older kernels we may just need to always return init_net,
	 * not sure when we added dev->nd_net.
	 */
	return dev->nd_net;
#else
	return &init_net;
#endif
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
#define netdev_notifier_info_to_dev(ndev) ndev
#endif

#endif /* __BACKPORT_NETDEVICE_H */
