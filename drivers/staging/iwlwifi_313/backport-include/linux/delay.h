#ifndef __BACKPORT_LINUX_DELAY_H
#define __BACKPORT_LINUX_DELAY_H
#include_next <linux/delay.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
#define usleep_range(_min, _max)	msleep((_max) / 1000)
#endif

#endif /* __BACKPORT_LINUX_DELAY_H */
