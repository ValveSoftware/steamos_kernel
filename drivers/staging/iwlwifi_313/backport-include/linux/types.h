#ifndef __BACKPORT_TYPES_H
#define __BACKPORT_TYPES_H
#include_next <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)

#if defined(CONFIG_X86) || defined(CONFIG_X86_64) || defined(CONFIG_PPC)
/*
 * CONFIG_PHYS_ADDR_T_64BIT was added as new to all architectures
 * as of 2.6.28 but x86 and ppc had it already.
 */
#else
#if defined(CONFIG_64BIT) || defined(CONFIG_X86_PAE) || defined(CONFIG_PPC64) || defined(CONFIG_PHYS_64BIT)
#define CONFIG_PHYS_ADDR_T_64BIT 1
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

#endif /* non x86 and ppc */

#endif /* < 2.6.28 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29) && \
	(defined(CONFIG_ALPHA) || defined(CONFIG_AVR32) || \
	 defined(CONFIG_BLACKFIN) || defined(CONFIG_CRIS) || \
	 defined(CONFIG_H8300) || defined(CONFIG_IA64) || \
	 defined(CONFIG_M68K) ||  defined(CONFIG_MIPS) || \
	 defined(CONFIG_PARISC) || defined(CONFIG_S390) || \
	 defined(CONFIG_PPC64) || defined(CONFIG_PPC32) || \
	 defined(CONFIG_SUPERH) || defined(CONFIG_SPARC) || \
	 defined(CONFIG_FRV) || defined(CONFIG_X86) || \
	 defined(CONFIG_M32R) || defined(CONFIG_M68K) || \
	 defined(CONFIG_MN10300) || defined(CONFIG_XTENSA) || \
	 defined(CONFIG_ARM))
#include <asm/atomic.h>
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
typedef struct {
	volatile int counter;
} atomic_t;

#ifdef CONFIG_64BIT
typedef struct {
	volatile long counter;
} atomic64_t;
#endif /* CONFIG_64BIT */

#endif

#endif /* __BACKPORT_TYPES_H */
