#ifndef __BACKPORT_LINUX_WORKQUEUE_H
#define __BACKPORT_LINUX_WORKQUEUE_H
#include_next <linux/workqueue.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0))
#define mod_delayed_work LINUX_BACKPORT(mod_delayed_work)
bool mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork,
		      unsigned long delay);
#endif

#ifndef create_freezable_workqueue
/* note freez_a_ble -> freez_ea_able */
#define create_freezable_workqueue create_freezeable_workqueue
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,36)
#define WQ_HIGHPRI 0
#define WQ_MEM_RECLAIM 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
#define WQ_UNBOUND	0
#endif
#define __WQ_ORDERED	0
/*
 * commit b196be89cdc14a88cc637cdad845a75c5886c82d
 * Author: Tejun Heo <tj@kernel.org>
 * Date:   Tue Jan 10 15:11:35 2012 -0800
 *
 *     workqueue: make alloc_workqueue() take printf fmt and args for name
 */
struct workqueue_struct *
backport_alloc_workqueue(const char *fmt, unsigned int flags,
			 int max_active, struct lock_class_key *key,
			 const char *lock_name, ...);
#undef alloc_workqueue
#ifdef CONFIG_LOCKDEP
#define alloc_workqueue(fmt, flags, max_active, args...)		\
({									\
	static struct lock_class_key __key;				\
	const char *__lock_name;					\
									\
	if (__builtin_constant_p(fmt))					\
		__lock_name = (fmt);					\
	else								\
		__lock_name = #fmt;					\
									\
	backport_alloc_workqueue((fmt), (flags), (max_active),		\
				 &__key, __lock_name, ##args);		\
})
#else
#define alloc_workqueue(fmt, flags, max_active, args...)		\
	backport_alloc_workqueue((fmt), (flags), (max_active),		\
				 NULL, NULL, ##args)
#endif
#undef alloc_ordered_workqueue
#define alloc_ordered_workqueue(fmt, flags, args...) \
	alloc_workqueue(fmt, WQ_UNBOUND | __WQ_ORDERED | (flags), 1, ##args)
#define destroy_workqueue backport_destroy_workqueue
void backport_destroy_workqueue(struct workqueue_struct *wq);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
#define system_wq LINUX_BACKPORT(system_wq)
extern struct workqueue_struct *system_wq;
#define system_long_wq LINUX_BACKPORT(system_long_wq)
extern struct workqueue_struct *system_long_wq;
#define system_nrt_wq LINUX_BACKPORT(system_nrt_wq)
extern struct workqueue_struct *system_nrt_wq;

void backport_system_workqueue_create(void);
void backport_system_workqueue_destroy(void);

#define schedule_work LINUX_BACKPORT(schedule_work)
int schedule_work(struct work_struct *work);
#define schedule_delayed_work LINUX_BACKPORT(schedule_delayed_work)
int schedule_delayed_work(struct delayed_work *dwork,
			  unsigned long delay);
#define flush_scheduled_work LINUX_BACKPORT(flush_scheduled_work)
void flush_scheduled_work(void);

#else

static inline void backport_system_workqueue_create(void)
{
}

static inline void backport_system_workqueue_destroy(void)
{
}
#endif /* < 2.6.36 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
/* I can't find a more suitable replacement... */
#define flush_work(work) cancel_work_sync(work)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
static inline void flush_delayed_work(struct delayed_work *dwork)
{
	if (del_timer_sync(&dwork->timer)) {
		/*
		 * This is what would happen on 2.6.32 but since we don't have
		 * access to the singlethread_cpu we can't really backport this,
		 * so avoid really *flush*ing the work... Oh well. Any better ideas?

		struct cpu_workqueue_struct *cwq;
		cwq = wq_per_cpu(keventd_wq, get_cpu());
		__queue_work(cwq, &dwork->work);
		put_cpu();

		*/
	}
	flush_work(&dwork->work);
}
#endif

#endif /* __BACKPORT_LINUX_WORKQUEUE_H */
