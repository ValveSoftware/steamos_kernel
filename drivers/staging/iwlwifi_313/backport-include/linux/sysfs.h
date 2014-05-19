#ifndef __BACKPORT_LINUX_SYSFS_H
#define __BACKPORT_LINUX_SYSFS_H
#include_next <linux/sysfs.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#ifdef CONFIG_DEBUG_LOCK_ALLOC
#define sysfs_attr_init(attr)				\
do {							\
	static struct lock_class_key __key;		\
							\
	(attr)->key = &__key;				\
} while(0)
#else
#define sysfs_attr_init(attr) do {} while(0)
#endif

/* mask sysfs_bin_attr_init as RHEL6 backports this */
#if !defined(sysfs_bin_attr_init)
/**
 *	sysfs_bin_attr_init - initialize a dynamically allocated bin_attribute
 *	@attr: struct bin_attribute to initialize
 *
 *	Initialize a dynamically allocated struct bin_attribute so we
 *	can make lockdep happy.  This is a new requirement for
 *	attributes and initially this is only needed when lockdep is
 *	enabled.  Lockdep gives a nice error when your attribute is
 *	added to sysfs if you don't have this.
 */
#define sysfs_bin_attr_init(bin_attr) sysfs_attr_init(&(bin_attr)->attr)
#endif
#endif

#ifndef __ATTR_RW
#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),		\
			 _name##_show, _name##_store)
#endif

#endif /* __BACKPORT_LINUX_SYSFS_H */
