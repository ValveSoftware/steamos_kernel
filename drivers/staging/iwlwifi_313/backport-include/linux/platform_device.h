#ifndef __BACKPORT_PLATFORM_DEVICE_H
#define __BACKPORT_PLATFORM_DEVICE_H

#include_next <linux/platform_device.h>
#include <linux/version.h>

#ifndef module_platform_driver_probe
#define module_platform_driver_probe(__platform_driver, __platform_probe) \
static int __init __platform_driver##_init(void) \
{ \
	return platform_driver_probe(&(__platform_driver), \
				     __platform_probe);    \
} \
module_init(__platform_driver##_init); \
static void __exit __platform_driver##_exit(void) \
{ \
	platform_driver_unregister(&(__platform_driver)); \
} \
module_exit(__platform_driver##_exit);
#endif

#ifndef PLATFORM_DEVID_NONE
#define PLATFORM_DEVID_NONE	(-1)
#endif

#ifndef PLATFORM_DEVID_AUTO
#define PLATFORM_DEVID_AUTO	(-1)
#endif

#ifndef module_platform_driver
#define module_platform_driver(__platform_driver) \
        module_driver(__platform_driver, platform_driver_register, \
                        platform_driver_unregister)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#define platform_device_register_data LINUX_BACKPORT(platform_device_register_data)
extern struct platform_device *platform_device_register_data(struct device *,
		const char *, int, const void *, size_t);
#endif

#endif /* __BACKPORT_PLATFORM_DEVICE_H */
