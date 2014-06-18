#ifndef __BACKPORT_PM_H
#define __BACKPORT_PM_H
#include_next <linux/pm.h>

#ifndef PM_EVENT_AUTO
#define PM_EVENT_AUTO		0x0400
#endif

#ifndef PM_EVENT_SLEEP
#define PM_EVENT_SLEEP  (PM_EVENT_SUSPEND)
#endif

#ifndef PMSG_IS_AUTO
#define PMSG_IS_AUTO(msg)	(((msg).event & PM_EVENT_AUTO) != 0)
#endif

#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,32)
#undef SIMPLE_DEV_PM_OPS
#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn) \
const struct dev_pm_ops name = { \
	.suspend = suspend_fn, \
	.resume = resume_fn, \
	.freeze = suspend_fn, \
	.thaw = resume_fn, \
	.poweroff = suspend_fn, \
	.restore = resume_fn, \
}
#endif /* 2.6.32 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
/*
 * dev_pm_ops is only available on kernels >= 2.6.29, for
 * older kernels we rely on reverting the work to old
 * power management style stuff. On 2.6.29 the pci calls
 * weren't included yet though, so include them here.
 */
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,29)
#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn)		\
struct dev_pm_ops name = {					\
	.suspend = suspend_fn ## _compat,			\
	.resume = resume_fn ## _compat,				\
	.freeze = suspend_fn ## _compat,			\
	.thaw = resume_fn ## _compat,				\
	.poweroff = suspend_fn ## _compat,			\
	.restore = resume_fn ## _compat,			\
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn) \
struct dev_pm_ops name = { \
	.suspend = suspend_fn, \
	.resume = resume_fn, \
	.freeze = suspend_fn, \
	.thaw = resume_fn, \
	.poweroff = suspend_fn, \
	.restore = resume_fn, \
}
#else
#define ___BACKPORT_PASTE(a, b) a##b
#define __BACKPORT_PASTE(a, b) ___BACKPORT_PASTE(a,b)
#define SIMPLE_DEV_PM_OPS(name, suspend_fn, resume_fn) \
	struct {} __maybe_unused __BACKPORT_PASTE(__backport_avoid_warning_, __LINE__)
#endif /* >= 2.6.29 */
#endif /* < 2.6.32 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
enum dpm_order {
	DPM_ORDER_NONE,
	DPM_ORDER_DEV_AFTER_PARENT,
	DPM_ORDER_PARENT_BEFORE_DEV,
	DPM_ORDER_DEV_LAST,
};
#endif

#endif /* __BACKPORT_PM_H */
