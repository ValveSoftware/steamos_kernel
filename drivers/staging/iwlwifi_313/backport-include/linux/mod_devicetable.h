#ifndef __BACKPORT_MOD_DEVICETABLE_H
#define __BACKPORT_MOD_DEVICETABLE_H
#include_next <linux/mod_devicetable.h>

#ifndef HID_BUS_ANY
#define HID_BUS_ANY                            0xffff
#endif

#ifndef HID_GROUP_ANY
#define HID_GROUP_ANY                          0x0000
#endif

#ifndef HID_ANY_ID
#define HID_ANY_ID                             (~0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
struct hid_device_id {
	__u16 bus;
	__u32 vendor;
	__u32 product;
	kernel_ulong_t driver_data
		__attribute__((aligned(sizeof(kernel_ulong_t))));
};
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#ifndef BCMA_CORE
/* Broadcom's specific AMBA core, see drivers/bcma/ */
struct bcma_device_id {
	__u16	manuf;
	__u16	id;
	__u8	rev;
	__u8	class;
};
#define BCMA_CORE(_manuf, _id, _rev, _class)  \
	{ .manuf = _manuf, .id = _id, .rev = _rev, .class = _class, }
#define BCMA_CORETABLE_END  \
	{ 0, },

#define BCMA_ANY_MANUF		0xFFFF
#define BCMA_ANY_ID		0xFFFF
#define BCMA_ANY_REV		0xFF
#define BCMA_ANY_CLASS		0xFF
#endif /* BCMA_CORE */

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)) */

#endif /* __BACKPORT_MOD_DEVICETABLE_H */
