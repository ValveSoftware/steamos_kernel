#ifndef __BACKPORT_LINUX_DMA_MAPPING_H
#define __BACKPORT_LINUX_DMA_MAPPING_H
#include_next <linux/dma-mapping.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
#define dma_zalloc_coherent LINUX_BACKPORT(dma_zalloc_coherent)
static inline void *dma_zalloc_coherent(struct device *dev, size_t size,
					dma_addr_t *dma_handle, gfp_t flag)
{
	void *ret = dma_alloc_coherent(dev, size, dma_handle, flag);
	if (ret)
		memset(ret, 0, size);
	return ret;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
/* only include this if DEFINE_DMA_UNMAP_ADDR is not set as debian squeeze also backports this  */
#ifndef DEFINE_DMA_UNMAP_ADDR
#ifdef CONFIG_NEED_DMA_MAP_STATE
#define DEFINE_DMA_UNMAP_ADDR(ADDR_NAME)        dma_addr_t ADDR_NAME
#define DEFINE_DMA_UNMAP_LEN(LEN_NAME)          __u32 LEN_NAME
#define dma_unmap_addr(PTR, ADDR_NAME)           ((PTR)->ADDR_NAME)
#define dma_unmap_addr_set(PTR, ADDR_NAME, VAL)  (((PTR)->ADDR_NAME) = (VAL))
#define dma_unmap_len(PTR, LEN_NAME)             ((PTR)->LEN_NAME)
#define dma_unmap_len_set(PTR, LEN_NAME, VAL)    (((PTR)->LEN_NAME) = (VAL))
#else
#define DEFINE_DMA_UNMAP_ADDR(ADDR_NAME)
#define DEFINE_DMA_UNMAP_LEN(LEN_NAME)
#define dma_unmap_addr(PTR, ADDR_NAME)           (0)
#define dma_unmap_addr_set(PTR, ADDR_NAME, VAL)  do { } while (0)
#define dma_unmap_len(PTR, LEN_NAME)             (0)
#define dma_unmap_len_set(PTR, LEN_NAME, VAL)    do { } while (0)
#endif
#endif

/* mask dma_set_coherent_mask as debian squeeze also backports this */
#define dma_set_coherent_mask LINUX_BACKPORT(dma_set_coherent_mask)

static inline int dma_set_coherent_mask(struct device *dev, u64 mask)
{
	if (!dma_supported(dev, mask))
		return -EIO;
	dev->coherent_dma_mask = mask;
	return 0;
}
#endif /* < 2.6.34 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#include <backport/magic.h>
/* These really belong to asm/dma-mapping.h but it doesn't really matter */
/* On 2.6.27 a second argument was added, on older kernels we ignore it */
static inline int dma_mapping_error1(dma_addr_t dma_addr)
{
	/* use an inline to grab the old definition */
	return dma_mapping_error(dma_addr);
}

#define dma_mapping_error2(pdef, dma_addr) \
	dma_mapping_error1(dma_addr)

#undef dma_mapping_error
#define dma_mapping_error(...) \
	macro_dispatcher(dma_mapping_error, __VA_ARGS__)(__VA_ARGS__)

/* This kinda belongs into asm/dma-mapping.h or so, but doesn't matter */
#ifdef CONFIG_ARM

/*
 * The caller asks to handle a range between offset and offset + size,
 * but we process a larger range from 0 to offset + size due to lack of
 * offset support.
 */

static inline void dma_sync_single_range_for_cpu(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(dev, handle, offset + size, dir);
}

static inline void dma_sync_single_range_for_device(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	dma_sync_single_for_device(dev, handle, offset + size, dir);
}

#endif /* arm */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
/*
 * Set both the DMA mask and the coherent DMA mask to the same thing.
 * Note that we don't check the return value from dma_set_coherent_mask()
 * as the DMA API guarantees that the coherent DMA mask can be set to
 * the same or smaller than the streaming DMA mask.
 */
static inline int dma_set_mask_and_coherent(struct device *dev, u64 mask)
{
	int rc = dma_set_mask(dev, mask);
	if (rc == 0)
		dma_set_coherent_mask(dev, mask);
	return rc;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0) */

#endif /* __BACKPORT_LINUX_DMA_MAPPING_H */
