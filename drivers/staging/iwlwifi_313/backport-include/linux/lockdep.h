#ifndef __BACKPORT_LINUX_LOCKDEP_H
#define __BACKPORT_LINUX_LOCKDEP_H
#include_next <linux/lockdep.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
/* Backport of:
 *
 * commit e159489baa717dbae70f9903770a6a4990865887
 * Author: Tejun Heo <tj@kernel.org>
 * Date:   Sun Jan 9 23:32:15 2011 +0100
 *
 *     workqueue: relax lockdep annotation on flush_work()
 */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
# ifdef CONFIG_PROVE_LOCKING
#  define lock_map_acquire_read(l)	lock_acquire(l, 0, 0, 2, 2, NULL, _THIS_IP_)
# else
#  define lock_map_acquire_read(l)	lock_acquire(l, 0, 0, 2, 1, NULL, _THIS_IP_)
# endif
#else
# define lock_map_acquire_read(l)		do { } while (0)
#endif

#endif /* < 2.6.38 */

#ifndef lockdep_assert_held
#define lockdep_assert_held(l)			do { } while (0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
/* Backport of:
 *
 * commit 3295f0ef9ff048a4619ede597ad9ec9cab725654
 * Author: Ingo Molnar <mingo@elte.hu>
 * Date:   Mon Aug 11 10:30:30 2008 +0200
 *
 *     lockdep: rename map_[acquire|release]() => lock_map_[acquire|release]()
 */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
# ifdef CONFIG_PROVE_LOCKING
#  define lock_map_acquire(l)		lock_acquire(l, 0, 0, 0, 2, NULL, _THIS_IP_)
# else
#  define lock_map_acquire(l)		lock_acquire(l, 0, 0, 0, 1, NULL, _THIS_IP_)
# endif
# define lock_map_release(l)			lock_release(l, 1, _THIS_IP_)
#else
# define lock_map_acquire(l)			do { } while (0)
# define lock_map_release(l)			do { } while (0)
#endif

#endif /* < 2.6.27 */

#endif /* __BACKPORT_LINUX_LOCKDEP_H */
