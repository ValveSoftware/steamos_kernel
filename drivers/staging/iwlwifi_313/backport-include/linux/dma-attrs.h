#ifndef __BACKPORT_DMA_ATTR_H
#define __BACKPORT_DMA_ATTR_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include_next <linux/dma-attrs.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26) */
#endif /* __BACKPORT_DMA_ATTR_H */
