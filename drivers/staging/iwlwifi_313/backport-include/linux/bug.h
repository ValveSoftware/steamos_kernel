#ifndef __BACKPORT_LINUX_BUG_H
#define __BACKPORT_LINUX_BUG_H
#include_next <linux/bug.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
/* is defined there for older kernels */
#include <linux/kernel.h>
/* Backport of:
 *
 * commit 7ef88ad561457c0346355dfd1f53e503ddfde719
 * Author: Rusty Russell <rusty@rustcorp.com.au>
 * Date:   Mon Jan 24 14:45:10 2011 -0600
 *
 *     BUILD_BUG_ON: make it handle more cases
 */
#undef BUILD_BUG_ON
/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 *
 * The implementation uses gcc's reluctance to create a negative array, but
 * gcc (as of 4.4) only emits that error for obvious cases (eg. not arguments
 * to inline functions).  So as a fallback we use the optimizer; if it can't
 * prove the condition is false, it will cause a link error on the undefined
 * "__build_bug_on_failed".  This error message can be harder to track down
 * though, hence the two different methods.
 */
#ifndef __OPTIMIZE__
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#else
extern int __build_bug_on_failed;
#define BUILD_BUG_ON(condition)					\
	do {							\
		((void)sizeof(char[1 - 2*!!(condition)]));	\
		if (condition) __build_bug_on_failed = 1;	\
	} while(0)
#endif
#endif /* < 2.6.38 */

#endif /* __BACKPORT_LINUX_BUG_H */
