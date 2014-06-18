#ifndef __BACKPORT_SKBUFF_H
#define __BACKPORT_SKBUFF_H
#include_next <linux/skbuff.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28))
extern void v2_6_28_skb_add_rx_frag(struct sk_buff *skb, int i,
				    struct page *page,
				    int off, int size);

#define skb_add_rx_frag(skb, i, page, off, size, truesize) \
	v2_6_28_skb_add_rx_frag(skb, i, page, off, size)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)) && \
      (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,4)) && \
      !(defined(CONFIG_SUSE_KERNEL) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)))
#define skb_add_rx_frag(skb, i, page, off, size, truesize) \
	skb_add_rx_frag(skb, i, page, off, size)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
#define __pskb_copy LINUX_BACKPORT(__pskb_copy)
extern struct sk_buff *__pskb_copy(struct sk_buff *skb,
				   int headroom, gfp_t gfp_mask);

#define skb_complete_wifi_ack LINUX_BACKPORT(skb_complete_wifi_ack)
static inline void skb_complete_wifi_ack(struct sk_buff *skb, bool acked)
{
	WARN_ON(1);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
#include <linux/dma-mapping.h>

/* mask skb_frag_page as RHEL6 backports this */
#define skb_frag_page LINUX_BACKPORT(skb_frag_page)
static inline struct page *skb_frag_page(const skb_frag_t *frag)
{
	return frag->page;
}

#define skb_frag_size LINUX_BACKPORT(skb_frag_size)
static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
	return frag->size;
}

/* mask skb_frag_dma_map as RHEL6 backports this */
#define skb_frag_dma_map LINUX_BACKPORT(skb_frag_dma_map)
static inline dma_addr_t skb_frag_dma_map(struct device *dev,
					  const skb_frag_t *frag,
					  size_t offset, size_t size,
					  enum dma_data_direction dir)
{
	return dma_map_page(dev, skb_frag_page(frag),
			    frag->page_offset + offset, size, dir);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
/* mask __netdev_alloc_skb_ip_align as RHEL6 backports this */
#define __netdev_alloc_skb_ip_align(a,b,c) compat__netdev_alloc_skb_ip_align(a,b,c)
static inline struct sk_buff *__netdev_alloc_skb_ip_align(struct net_device *dev,
							  unsigned int length, gfp_t gfp)
{
	struct sk_buff *skb = __netdev_alloc_skb(dev, length + NET_IP_ALIGN, gfp);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
#define skb_checksum_start_offset LINUX_BACKPORT(skb_checksum_start_offset)
static inline int skb_checksum_start_offset(const struct sk_buff *skb)
{
	return skb->csum_start - skb_headroom(skb);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#define skb_has_frag_list LINUX_BACKPORT(skb_has_frag_list)
static inline bool skb_has_frag_list(const struct sk_buff *skb)
{
	return skb_shinfo(skb)->frag_list != NULL;
}

#define skb_checksum_none_assert LINUX_BACKPORT(skb_checksum_none_assert)

static inline void skb_checksum_none_assert(struct sk_buff *skb)
{
#ifdef DEBUG
	BUG_ON(skb->ip_summed != CHECKSUM_NONE);
#endif
}
#endif /* < 2.6.37 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static inline bool skb_defer_rx_timestamp(struct sk_buff *skb)
{
	return false;
}

#if (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,4))
static inline void skb_tx_timestamp(struct sk_buff *skb)
{
}
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
/* mask netdev_alloc_skb_ip_align as debian squeeze also backports this */
#define netdev_alloc_skb_ip_align LINUX_BACKPORT(netdev_alloc_skb_ip_align)

static inline struct sk_buff *netdev_alloc_skb_ip_align(struct net_device *dev,
                unsigned int length)
{
	struct sk_buff *skb = netdev_alloc_skb(dev, length + NET_IP_ALIGN);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}
#endif

#ifndef skb_walk_frags
#define skb_walk_frags(skb, iter)	\
	for (iter = skb_shinfo(skb)->frag_list; iter; iter = iter->next)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static inline bool skb_queue_is_first(const struct sk_buff_head *list,
				      const struct sk_buff *skb)
{
	return (skb->prev == (struct sk_buff *) list);
}

static inline struct sk_buff *skb_queue_prev(const struct sk_buff_head *list,
					     const struct sk_buff *skb)
{
	BUG_ON(skb_queue_is_first(list, skb));
	return skb->prev;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static inline bool skb_queue_is_last(const struct sk_buff_head *list,
				     const struct sk_buff *skb)
{
	return (skb->next == (struct sk_buff *) list);
}

static inline struct sk_buff *skb_queue_next(const struct sk_buff_head *list,
                                             const struct sk_buff *skb)
{
	/* This BUG_ON may seem severe, but if we just return then we
	 * are going to dereference garbage.
	 */
	BUG_ON(skb_queue_is_last(list, skb));
	return skb->next;
}

static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
	list->prev = list->next = (struct sk_buff *)list;
	list->qlen = 0;
}

static inline void __skb_queue_splice(const struct sk_buff_head *list,
				      struct sk_buff *prev,
				      struct sk_buff *next)
{
	struct sk_buff *first = list->next;
	struct sk_buff *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void skb_queue_splice(const struct sk_buff_head *list,
				    struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, (struct sk_buff *) head, head->next);
		head->qlen += list->qlen;
	}
}

static inline void skb_queue_splice_init(struct sk_buff_head *list,
					 struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, (struct sk_buff *) head, head->next);
		head->qlen += list->qlen;
		__skb_queue_head_init(list);
	}
}

static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
					      struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, head->prev, (struct sk_buff *) head);
		head->qlen += list->qlen;
		__skb_queue_head_init(list);
	}
}

static inline void skb_queue_splice_tail(const struct sk_buff_head *list,
					 struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, head->prev, (struct sk_buff *) head);
		head->qlen += list->qlen;
	}
}

#define skb_queue_walk_from(queue, skb)						\
		for (; skb != (struct sk_buff *)(queue);			\
		     skb = skb->next)
#endif /* < 2.6.28 */

#endif /* __BACKPORT_SKBUFF_H */
