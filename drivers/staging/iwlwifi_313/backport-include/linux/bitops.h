#ifndef __BACKPORT_BITOPS_H
#define __BACKPORT_BITOPS_H
#include_next <linux/bitops.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
static inline unsigned long __ffs64(u64 word)
{
#if BITS_PER_LONG == 32
	if (((u32)word) == 0UL)
		return __ffs((u32)(word >> 32)) + 32;
#elif BITS_PER_LONG != 64
#error BITS_PER_LONG not 32 or 64
#endif
	return __ffs((unsigned long)word);
}
#endif /* < 2.6.30 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))
#endif /* < 2.6.34 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#define sign_extend32 LINUX_BACKPORT(sign_extend32)
static inline __s32 sign_extend32(__u32 value, int index)
{
	__u8 shift = 31 - index;
	return (__s32)(value << shift) >> shift;
}
#endif /* < 2.6.38 */

#endif /* __BACKPORT_BITOPS_H */
