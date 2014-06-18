#ifndef __BACKPORT_ASM_ATOMIC_H
#define __BACKPORT_ASM_ATOMIC_H
#include_next <asm/atomic.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
/*
 * In many versions, several architectures do not seem to include an
 * atomic64_t implementation, and do not include the software emulation from
 * asm-generic/atomic64_t.
 * Detect and handle this here.
 */
#include <asm/atomic.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31)) && !defined(ATOMIC64_INIT) && !defined(CONFIG_X86) && !((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)) && defined(CONFIG_ARM) && !defined(CONFIG_GENERIC_ATOMIC64))
#include <asm-generic/atomic64.h>
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
#ifndef CONFIG_64BIT

typedef struct {
	long long counter;
} atomic64_t;

#define atomic64_read LINUX_BACKPORT(atomic64_read)
extern long long atomic64_read(const atomic64_t *v);
#define atomic64_add_return LINUX_BACKPORT(atomic64_add_return)
extern long long atomic64_add_return(long long a, atomic64_t *v);

#define atomic64_inc_return(v)          atomic64_add_return(1LL, (v))

#endif
#endif

#endif /* __BACKPORT_ASM_ATOMIC_H */
