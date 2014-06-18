#ifndef LINUX_26_34_COMPAT_PRIVATE_H
#define LINUX_26_34_COMPAT_PRIVATE_H

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))

#include <linux/mmc/sdio_func.h>

void backport_init_mmc_pm_flags(void);

#else /* Kernels >= 2.6.34 */

static inline void backport_init_mmc_pm_flags(void)
{
}

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)) */

#endif /* LINUX_26_34_COMPAT_PRIVATE_H */
