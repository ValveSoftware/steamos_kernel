#ifndef __BACKPORT_LINUX_STRING_H
#define __BACKPORT_LINUX_STRING_H
#include_next <linux/string.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
#define memweight LINUX_BACKPORT(memweight)
extern size_t memweight(const void *ptr, size_t bytes);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#define strtobool LINUX_BACKPORT(strtobool)
extern int strtobool(const char *s, bool *res);
#endif

#endif /* __BACKPORT_LINUX_STRING_H */
