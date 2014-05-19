#ifndef _COMPAT_LINUX_MATH64_H
#define _COMPAT_LINUX_MATH64_H 1

#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25))
#include_next <linux/math64.h>
#else
#include <linux/types.h>
#include <asm/div64.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25)) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#if BITS_PER_LONG == 64

static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

#elif BITS_PER_LONG == 32

#ifndef div_u64_rem
static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = do_div(dividend, divisor);
	return dividend;
}
#endif

#endif /* BITS_PER_LONG */

#ifndef div_u64
static inline u64 div_u64(u64 dividend, u32 divisor)
{
	u32 remainder;
	return div_u64_rem(dividend, divisor, &remainder);
}
#endif

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26) */

#endif	/* _COMPAT_LINUX_MATH64_H */
