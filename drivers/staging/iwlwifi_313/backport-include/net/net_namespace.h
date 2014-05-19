#ifndef _COMPAT_NET_NET_NAMESPACE_H
#define _COMPAT_NET_NET_NAMESPACE_H 1

#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23))
#include_next <net/net_namespace.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
#ifdef CONFIG_NET_NS
static inline void write_pnet(struct net **pnet, struct net *net)
{
	*pnet = net;
}

static inline struct net *read_pnet(struct net * const *pnet)
{
	return *pnet;
}

#else
#define write_pnet(pnet, net)	do { (void)(net);} while (0)
#define read_pnet(pnet)		(&init_net)
#endif
#endif /* < 2.6.29 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#ifdef CONFIG_NET_NS
static inline
int net_eq(const struct net *net1, const struct net *net2)
{
	return net1 == net2;
}
#else
static inline
int net_eq(const struct net *net1, const struct net *net2)
{
	return 1;
}
#endif

static inline
void dev_net_set(struct net_device *dev, struct net *net)
{
#ifdef CONFIG_NET_NS
	release_net(dev->nd_net);
	dev->nd_net = hold_net(net);
#endif
}

static inline
struct net *sock_net(const struct sock *sk)
{
#ifdef CONFIG_NET_NS
	return sk->sk_net;
#else
	return &init_net;
#endif
}
#endif /* < 2.6.26 */

#endif	/* _COMPAT_NET_NET_NAMESPACE_H */
