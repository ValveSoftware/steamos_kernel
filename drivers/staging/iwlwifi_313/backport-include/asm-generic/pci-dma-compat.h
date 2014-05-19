#ifndef __BACKPORT_ASM_PCI_DMA_COMPAT_H
#define __BACKPORT_ASM_PCI_DMA_COMPAT_H
#include_next <asm-generic/pci-dma-compat.h>
#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#include <backport/magic.h>

#define pci_dma_mapping_error1(dma_addr) dma_mapping_error1(dma_addr)
#define pci_dma_mapping_error2(pdev, dma_addr) dma_mapping_error2(pdev, dma_addr)
#undef pci_dma_mapping_error
#define pci_dma_mapping_error(...) \
	macro_dispatcher(pci_dma_mapping_error, __VA_ARGS__)(__VA_ARGS__)
#endif

#endif /* __BACKPORT_ASM_PCI_DMA_COMPAT_H */
