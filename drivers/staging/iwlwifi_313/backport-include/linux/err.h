#ifndef __BACKPORT_LINUX_ERR_H
#define __BACKPORT_LINUX_ERR_H
#include_next <linux/err.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
#define PTR_RET LINUX_BACKPORT(PTR_RET)
static inline int __must_check PTR_RET(const void *ptr)
{
	if (IS_ERR(ptr))
		return PTR_ERR(ptr);
	else
		return 0;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
/* mask IS_ERR_OR_NULL as debian squeeze also backports this */
#define IS_ERR_OR_NULL LINUX_BACKPORT(IS_ERR_OR_NULL)

static inline long __must_check IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
#define PTR_ERR_OR_ZERO(p) PTR_RET(p)
#endif

#endif /* __BACKPORT_LINUX_ERR_H */
