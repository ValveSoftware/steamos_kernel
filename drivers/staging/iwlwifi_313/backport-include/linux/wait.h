#ifndef __BACKPORT_LINUX_WAIT_H
#define __BACKPORT_LINUX_WAIT_H
#include_next <linux/wait.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#define wake_up_interruptible_poll(x, m)			\
	__wake_up(x, TASK_INTERRUPTIBLE, 1, (void *) (m))
#endif

#endif /* __BACKPORT_LINUX_WAIT_H */
