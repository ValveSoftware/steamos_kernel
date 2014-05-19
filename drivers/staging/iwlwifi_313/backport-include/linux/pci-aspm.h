#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
#include_next <linux/pci-aspm.h>
#else
#define PCIE_LINK_STATE_L0S	1
#define PCIE_LINK_STATE_L1	2
#define PCIE_LINK_STATE_CLKPM	4

static inline void pci_disable_link_state(struct pci_dev *pdev, int state)
{
}
#endif
