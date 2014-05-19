#ifndef LINUX_BACKPORTS_PRIVATE_H
#define LINUX_BACKPORTS_PRIVATE_H

#include <linux/version.h>

#ifdef CPTCFG_BACKPORT_BUILD_DMA_SHARED_BUFFER
int __init dma_buf_init(void);
void __exit dma_buf_deinit(void);
#else
static inline int __init dma_buf_init(void) { return 0; }
static inline void __exit dma_buf_deinit(void) { }
#endif

#endif /* LINUX_BACKPORTS_PRIVATE_H */
