#ifndef __BACKPORT_LINUX_RTNETLINK_H
#define __BACKPORT_LINUX_RTNETLINK_H
#include_next <linux/rtnetlink.h>

#ifndef rtnl_dereference
#define rtnl_dereference(p)                                     \
        rcu_dereference_protected(p, lockdep_rtnl_is_held())
#endif

#ifndef rcu_dereference_rtnl
#define rcu_dereference_rtnl(p)					\
	rcu_dereference_check(p, rcu_read_lock_held() ||	\
				 lockdep_rtnl_is_held())
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#ifdef CONFIG_PROVE_LOCKING
/*
 * Obviously, this is wrong.  But the base kernel will have rtnl_mutex
 * declared static, with no way to access it.  I think this is the best
 * we can do...
 */
static inline int lockdep_rtnl_is_held(void)
{
        return 1;
}
#endif /* #ifdef CONFIG_PROVE_LOCKING */
#endif /* < 2.6.34 */

#endif /* __BACKPORT_LINUX_RTNETLINK_H */
