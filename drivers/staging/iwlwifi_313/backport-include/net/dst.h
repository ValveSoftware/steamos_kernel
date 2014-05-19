#ifndef __BACKPORT_NET_DST_H
#define __BACKPORT_NET_DST_H
#include_next <net/dst.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
/*
 * Added via adf30907d63893e4208dfe3f5c88ae12bc2f25d5
 *
 * There is no _sk_dst on older kernels, so just set the
 * old dst to NULL and release it directly.
 */
static inline void skb_dst_drop(struct sk_buff *skb)
{
	dst_release(skb->dst);
	skb->dst = NULL;
}

static inline struct dst_entry *skb_dst(const struct sk_buff *skb)
{
	return (struct dst_entry *)skb->dst;
}

static inline void skb_dst_set(struct sk_buff *skb, struct dst_entry *dst)
{
	skb->dst = dst;
}

static inline struct rtable *skb_rtable(const struct sk_buff *skb)
{
	return (struct rtable *)skb_dst(skb);
}
#endif

#endif /* __BACKPORT_NET_DST_H */
