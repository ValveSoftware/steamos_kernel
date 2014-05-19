#ifndef __BACKPORT_LINUX_TIMER_H
#define __BACKPORT_LINUX_TIMER_H
#include_next <linux/timer.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#define round_jiffies_up LINUX_BACKPORT(round_jiffies_up)
unsigned long round_jiffies_up(unsigned long j);
#endif

#endif /* __BACKPORT_LINUX_TIMER_H */
