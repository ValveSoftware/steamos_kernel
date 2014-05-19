#ifndef __BACKPORT_COMPAT_H
#define __BACKPORT_COMPAT_H

#include_next <linux/compat.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0))
#ifdef CONFIG_X86_X32_ABI
#define COMPAT_USE_64BIT_TIME \
	(!!(task_pt_regs(current)->orig_ax & __X32_SYSCALL_BIT))
#else
#define COMPAT_USE_64BIT_TIME 0
#endif
#endif

#endif /* __BACKPORT_COMPAT_H */
