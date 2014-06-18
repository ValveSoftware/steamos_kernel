#ifndef _BACKPORT_LINUX_PCI_H
#define _BACKPORT_LINUX_PCI_H
#include_next <linux/pci.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
#define compat_pci_suspend(fn)						\
	int fn##_compat(struct pci_dev *pdev, pm_message_t state) 	\
	{								\
		int r;							\
									\
		r = fn(&pdev->dev);					\
		if (r)							\
			return r;					\
									\
		pci_save_state(pdev);					\
		pci_disable_device(pdev);				\
		pci_set_power_state(pdev, PCI_D3hot);			\
									\
		return 0;						\
	}

#define compat_pci_resume(fn)						\
	int fn##_compat(struct pci_dev *pdev)				\
	{								\
		int r;							\
									\
		pci_set_power_state(pdev, PCI_D0);			\
		r = pci_enable_device(pdev);				\
		if (r)							\
			return r;					\
		pci_restore_state(pdev);				\
									\
		return fn(&pdev->dev);					\
	}
#elif LINUX_VERSION_CODE == KERNEL_VERSION(2,6,29)
#define compat_pci_suspend(fn)						\
	int fn##_compat(struct device *dev)			 	\
	{								\
		struct pci_dev *pdev = to_pci_dev(dev);			\
		int r;							\
									\
		r = fn(&pdev->dev);					\
		if (r)							\
			return r;					\
									\
		pci_save_state(pdev);					\
		pci_disable_device(pdev);				\
		pci_set_power_state(pdev, PCI_D3hot);			\
									\
		return 0;						\
	}

#define compat_pci_resume(fn)						\
	int fn##_compat(struct device *dev)				\
	{								\
		struct pci_dev *pdev = to_pci_dev(dev);			\
		int r;							\
									\
		pci_set_power_state(pdev, PCI_D0);			\
		r = pci_enable_device(pdev);				\
		if (r)							\
			return r;					\
		pci_restore_state(pdev);				\
									\
		return fn(&pdev->dev);					\
	}
#else
#define compat_pci_suspend(fn)
#define compat_pci_resume(fn)
#endif

#ifndef module_pci_driver
/**
 * module_pci_driver() - Helper macro for registering a PCI driver
 * @__pci_driver: pci_driver struct
 *
 * Helper macro for PCI drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
#define module_pci_driver(__pci_driver) \
	module_driver(__pci_driver, pci_register_driver, \
		       pci_unregister_driver)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
#define pcie_capability_read_word LINUX_BACKPORT(pcie_capability_read_word)
int pcie_capability_read_word(struct pci_dev *dev, int pos, u16 *val);
#define pcie_capability_read_dword LINUX_BACKPORT(pcie_capability_read_dword)
int pcie_capability_read_dword(struct pci_dev *dev, int pos, u32 *val);
#define pcie_capability_write_word LINUX_BACKPORT(pcie_capability_write_word)
int pcie_capability_write_word(struct pci_dev *dev, int pos, u16 val);
#define pcie_capability_write_dword LINUX_BACKPORT(pcie_capability_write_dword)
int pcie_capability_write_dword(struct pci_dev *dev, int pos, u32 val);
#define pcie_capability_clear_and_set_word LINUX_BACKPORT(pcie_capability_clear_and_set_word)
int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos,
				       u16 clear, u16 set);
#define pcie_capability_clear_and_set_dword LINUX_BACKPORT(pcie_capability_clear_and_set_dword)
int pcie_capability_clear_and_set_dword(struct pci_dev *dev, int pos,
					u32 clear, u32 set);

#define pcie_capability_set_word LINUX_BACKPORT(pcie_capability_set_word)
static inline int pcie_capability_set_word(struct pci_dev *dev, int pos,
					   u16 set)
{
	return pcie_capability_clear_and_set_word(dev, pos, 0, set);
}

#define pcie_capability_set_dword LINUX_BACKPORT(pcie_capability_set_dword)
static inline int pcie_capability_set_dword(struct pci_dev *dev, int pos,
					    u32 set)
{
	return pcie_capability_clear_and_set_dword(dev, pos, 0, set);
}

#define pcie_capability_clear_word LINUX_BACKPORT(pcie_capability_clear_word)
static inline int pcie_capability_clear_word(struct pci_dev *dev, int pos,
					     u16 clear)
{
	return pcie_capability_clear_and_set_word(dev, pos, clear, 0);
}

#define pcie_capability_clear_dword LINUX_BACKPORT(pcie_capability_clear_dword)
static inline int pcie_capability_clear_dword(struct pci_dev *dev, int pos,
					      u32 clear)
{
	return pcie_capability_clear_and_set_dword(dev, pos, clear, 0);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
/* mask pci_pcie_cap as debian squeeze also backports this */
#define pci_pcie_cap LINUX_BACKPORT(pci_pcie_cap)
static inline int pci_pcie_cap(struct pci_dev *dev)
{
	return pci_find_capability(dev, PCI_CAP_ID_EXP);
}

/* mask pci_is_pcie as RHEL6 backports this */
#define pci_is_pcie LINUX_BACKPORT(pci_is_pcie)
static inline bool pci_is_pcie(struct pci_dev *dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	return dev->is_pcie;
#else
	return !!pci_pcie_cap(dev);
#endif
}
#endif /* < 2.6.33 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#define pci_ioremap_bar LINUX_BACKPORT(pci_ioremap_bar)
void __iomem *pci_ioremap_bar(struct pci_dev *pdev, int bar);

#define pci_wake_from_d3 LINUX_BACKPORT(pci_wake_from_d3)
int pci_wake_from_d3(struct pci_dev *dev, bool enable);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define pci_pme_capable LINUX_BACKPORT(pci_pme_capable)
bool pci_pme_capable(struct pci_dev *dev, pci_power_t state);
#endif

#ifndef PCI_DEVICE_SUB
/**
 * PCI_DEVICE_SUB - macro used to describe a specific pci device with subsystem
 * @vend: the 16 bit PCI Vendor ID
 * @dev: the 16 bit PCI Device ID
 * @subvend: the 16 bit PCI Subvendor ID
 * @subdev: the 16 bit PCI Subdevice ID
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific device with subsystem information.
 */
#define PCI_DEVICE_SUB(vend, dev, subvend, subdev) \
	.vendor = (vend), .device = (dev), \
	.subvendor = (subvend), .subdevice = (subdev)
#endif /* PCI_DEVICE_SUB */

#endif /* _BACKPORT_LINUX_PCI_H */
