#ifndef __BACKPORT_NET_SCH_GENERIC_H
#define __BACKPORT_NET_SCH_GENERIC_H
#include_next <net/sch_generic.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
#if !((LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,9) && LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,23) && LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)))
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,37)
/* mask qdisc_cb_private_validate as RHEL6 backports this */
#define qdisc_cb_private_validate(a,b) compat_qdisc_cb_private_validate(a,b)
static inline void qdisc_cb_private_validate(const struct sk_buff *skb, int sz)
{
	BUILD_BUG_ON(sizeof(skb->cb) < sizeof(struct qdisc_skb_cb) + sz);
}
#else
/* mask qdisc_cb_private_validate as RHEL6 backports this */
#define qdisc_cb_private_validate(a,b) compat_qdisc_cb_private_validate(a,b)
static inline void qdisc_cb_private_validate(const struct sk_buff *skb, int sz)
{
	/* XXX ? */
}
#endif
#endif
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
static inline struct net_device *qdisc_dev(const struct Qdisc *qdisc)
{
	return qdisc->dev;
}

/*
 * Backports 378a2f09 and c27f339a
 * This may need a bit more work.
 */
enum net_xmit_qdisc_t {
	__NET_XMIT_STOLEN = 0x00010000,
	__NET_XMIT_BYPASS = 0x00020000,
};

struct qdisc_skb_cb {
	unsigned int            pkt_len;
	char                    data[];
};

static inline struct qdisc_skb_cb *qdisc_skb_cb(struct sk_buff *skb)
{
	return (struct qdisc_skb_cb *)skb->cb;
}

static inline unsigned int qdisc_pkt_len(struct sk_buff *skb)
{
	return qdisc_skb_cb(skb)->pkt_len;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static inline void bstats_update(struct gnet_stats_basic_packed *bstats,
				 const struct sk_buff *skb)
{
	bstats->bytes += qdisc_pkt_len((struct sk_buff *) skb);
	bstats->packets += skb_is_gso(skb) ? skb_shinfo(skb)->gso_segs : 1;
}
static inline void qdisc_bstats_update(struct Qdisc *sch,
				       const struct sk_buff *skb)
{
	bstats_update(&sch->bstats, skb);
}
#else
/*
 * kernels <= 2.6.30 do not pass a const skb to qdisc_pkt_len, and
 * gnet_stats_basic_packed did not exist (see c1a8f1f1c8)
 */
static inline void bstats_update(struct gnet_stats_basic *bstats,
				 struct sk_buff *skb)
{
	bstats->bytes += qdisc_pkt_len(skb);
	bstats->packets += skb_is_gso(skb) ? skb_shinfo(skb)->gso_segs : 1;
}
static inline void qdisc_bstats_update(struct Qdisc *sch,
				       struct sk_buff *skb)
{
	bstats_update(&sch->bstats, skb);
}
#endif
#endif /* < 2.6.38 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)

#define qdisc_reset_all_tx_gt LINUX_BACKPORT(qdisc_reset_all_tx_gt)

/* Reset all TX qdiscs greater then index of a device.  */
static inline void qdisc_reset_all_tx_gt(struct net_device *dev, unsigned int i)
{
	struct Qdisc *qdisc;

	for (; i < dev->num_tx_queues; i++) {
		qdisc = netdev_get_tx_queue(dev, i)->qdisc;
		if (qdisc) {
			spin_lock_bh(qdisc_lock(qdisc));
			qdisc_reset(qdisc);
			spin_unlock_bh(qdisc_lock(qdisc));
		}
	}
}
#else
static inline void qdisc_reset_all_tx_gt(struct net_device *dev, unsigned int i)
{
}
#endif /* >= 2.6.27 */
#endif /* < 2.6.35 */

#ifndef TCQ_F_CAN_BYPASS
#define TCQ_F_CAN_BYPASS        4
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
static inline int qdisc_qlen(const struct Qdisc *q)
{
	return q->q.qlen;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
static inline bool qdisc_all_tx_empty(const struct net_device *dev)
{
	return skb_queue_empty(&dev->qdisc->q);
}
#endif

#endif /* __BACKPORT_NET_SCH_GENERIC_H */
