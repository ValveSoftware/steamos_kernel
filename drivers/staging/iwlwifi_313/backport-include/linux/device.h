#ifndef __BACKPORT_DEVICE_H
#define __BACKPORT_DEVICE_H
#include <linux/export.h>
#include_next <linux/device.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
/* backport
 * commit 9f3b795a626ee79574595e06d1437fe0c7d51d29
 * Author: Michał Mirosław <mirq-linux@rere.qmqm.pl>
 * Date: Fri Feb 1 20:40:17 2013 +0100
 *
 * driver-core: constify data for class_find_device()
 */
typedef int (backport_device_find_function_t)(struct device *, void *);
#define class_find_device(cls, start, idx, fun) \
	class_find_device((cls), (start), (void *)(idx),\
			  (backport_device_find_function_t *)(fun))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static inline int
backport_device_move(struct device *dev, struct device *new_parent,
		     enum dpm_order dpm_order)
{
	return device_move(dev, new_parent);
}
#define device_move LINUX_BACKPORT(device_move)
#endif

#ifndef module_driver
/**
 * module_driver() - Helper macro for drivers that don't do anything
 * special in module init/exit. This eliminates a lot of boilerplate.
 * Each module may only use this macro once, and calling it replaces
 * module_init() and module_exit().
 *
 * Use this macro to construct bus specific macros for registering
 * drivers, and do not use it on its own.
 */
#define module_driver(__driver, __register, __unregister) \
static int __init __driver##_init(void) \
{ \
	return __register(&(__driver)); \
} \
module_init(__driver##_init); \
static void __exit __driver##_exit(void) \
{ \
	__unregister(&(__driver)); \
} \
module_exit(__driver##_exit);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
#define devm_ioremap_resource LINUX_BACKPORT(devm_ioremap_resource)
void __iomem *devm_ioremap_resource(struct device *dev, struct resource *res);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0) && \
    LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#define devres_release LINUX_BACKPORT(devres_release)
extern int devres_release(struct device *dev, dr_release_t release,
			  dr_match_t match, void *match_data);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0) && \
    LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/ratelimit.h>

#define dev_level_ratelimited(dev_level, dev, fmt, ...)			\
do {									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
	if (__ratelimit(&_rs))						\
		dev_level(dev, fmt, ##__VA_ARGS__);			\
} while (0)

#define dev_emerg_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_emerg, dev, fmt, ##__VA_ARGS__)
#define dev_alert_ratelimited(dev, fmt, ...)				\
	dev_level_ratelimited(dev_alert, dev, fmt, ##__VA_ARGS__)


#if defined(CONFIG_DYNAMIC_DEBUG) || defined(DEBUG)
#define dev_dbg_ratelimited(dev, fmt, ...)				\
do {									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
	DEFINE_DYNAMIC_DEBUG_METADATA(descriptor, fmt);			\
	if (unlikely(descriptor.flags & _DPRINTK_FLAGS_PRINT) &&	\
	    __ratelimit(&_rs))						\
		__dynamic_pr_debug(&descriptor, pr_fmt(fmt),		\
				   ##__VA_ARGS__);			\
} while (0)
#else
#define dev_dbg_ratelimited(dev, fmt, ...)			\
	no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#endif /* dynamic debug */
#endif /* 2.6.27 <= version <= 3.5 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
#define device_rename(dev, new_name) device_rename(dev, (char *)new_name)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
/*
 * This belongs into pm_wakeup.h but that isn't included directly.
 * Note that on 2.6.36, this was defined but not exported, so we
 * need to override it.
 */
#define pm_wakeup_event LINUX_BACKPORT(pm_wakeup_event)
static inline void pm_wakeup_event(struct device *dev, unsigned int msec) {}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#define device_lock LINUX_BACKPORT(device_lock)
static inline void device_lock(struct device *dev)
{
#if defined(CONFIG_PREEMPT_RT) || defined(CONFIG_PREEMPT_DESKTOP)
        mutex_lock(&dev->mutex);
#else
	down(&dev->sem);
#endif
}

#define device_trylock LINUX_BACKPORT(device_trylock)
static inline int device_trylock(struct device *dev)
{
#if defined(CONFIG_PREEMPT_RT) || defined(CONFIG_PREEMPT_DESKTOP)
	return mutex_trylock(&dev->mutex);
#else
	return down_trylock(&dev->sem);
#endif
}

#define device_unlock LINUX_BACKPORT(device_unlock)
static inline void device_unlock(struct device *dev)
{
#if defined(CONFIG_PREEMPT_RT) || defined(CONFIG_PREEMPT_DESKTOP)
        mutex_unlock(&dev->mutex);
#else
	up(&dev->sem);
#endif
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
static inline const char *dev_name(struct device *dev)
{
	/* will be changed into kobject_name(&dev->kobj) in the near future */
	return dev->bus_id;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static inline void dev_set_uevent_suppress(struct device *dev, int val)
{
	dev->uevent_suppress = val;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define device_create(cls, parent, devt, drvdata, fmt, ...)		\
({									\
	struct device *_dev;						\
	_dev = (device_create)(cls, parent, devt, fmt, __VA_ARGS__);	\
	dev_set_drvdata(_dev, drvdata);					\
	_dev;								\
})

#define dev_name(dev) dev_name((struct device *)dev)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#define dev_set_name LINUX_BACKPORT(dev_set_name)
extern int dev_set_name(struct device *dev, const char *name, ...)
			__attribute__((format(printf, 2, 3)));
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,6,0)
static inline void
backport_device_release_driver(struct device *dev)
{
	device_release_driver(dev);
	device_lock(dev);
	dev_set_drvdata(dev, NULL);
	device_unlock(dev);
}
#define device_release_driver LINUX_BACKPORT(device_release_driver)
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(3,6,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
#define DEVICE_ATTR_RO(_name) \
struct device_attribute dev_attr_ ## _name = __ATTR_RO(_name);
#define DEVICE_ATTR_RW(_name) \
struct device_attribute dev_attr_ ## _name = __ATTR_RW(_name)
#endif

#define ATTRIBUTE_GROUPS_BACKPORT(_name) \
static struct BP_ATTR_GRP_STRUCT _name##_dev_attrs[ARRAY_SIZE(_name##_attrs)];\
static void init_##_name##_attrs(void)				\
{									\
	int i;								\
	for (i = 0; _name##_attrs[i]; i++)				\
		_name##_dev_attrs[i] =				\
			*container_of(_name##_attrs[i],		\
				      struct BP_ATTR_GRP_STRUCT,	\
				      attr);				\
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)
#undef ATTRIBUTE_GROUPS
#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
static inline void init_##_name##_attrs(void) {}		\
__ATTRIBUTE_GROUPS(_name)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#define dev_get_platdata LINUX_BACKPORT(dev_get_platdata)
static inline void *dev_get_platdata(const struct device *dev)
{
	return dev->platform_data;
}
#endif

#endif /* __BACKPORT_DEVICE_H */
