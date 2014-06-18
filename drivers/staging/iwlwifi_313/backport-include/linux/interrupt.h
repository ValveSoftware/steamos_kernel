#ifndef __BACKPORT_LINUX_INTERRUPT_H
#define __BACKPORT_LINUX_INTERRUPT_H
#include_next <linux/interrupt.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
static inline int irq_set_irq_wake(unsigned int irq, unsigned int on)
{
	return set_irq_wake(irq, on);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#define IRQ_WAKE_THREAD	(2)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
struct compat_threaded_irq {
	unsigned int irq;
	irq_handler_t handler;
	irq_handler_t thread_fn;
	void *dev_id;
	char wq_name[64];
	struct workqueue_struct *wq;
	struct work_struct work;
};

static inline
void compat_irq_work(struct work_struct *work)
{
	struct compat_threaded_irq *comp = container_of(work, struct compat_threaded_irq, work);
	comp->thread_fn(comp->irq, comp->dev_id);
}

static inline
irqreturn_t compat_irq_dispatcher(int irq, void *dev_id)
{
	struct compat_threaded_irq *comp = dev_id;
	irqreturn_t res;

	res = comp->handler(irq, comp->dev_id);
	if (res == IRQ_WAKE_THREAD) {
		queue_work(comp->wq, &comp->work);
		res = IRQ_HANDLED;
	}

	return res;
}

static inline
int compat_request_threaded_irq(struct compat_threaded_irq *comp,
				unsigned int irq,
				irq_handler_t handler,
				irq_handler_t thread_fn,
				unsigned long flags,
				const char *name,
				void *dev_id)
{
	comp->irq = irq;
	comp->handler = handler;
	comp->thread_fn = thread_fn;
	comp->dev_id = dev_id;
	INIT_WORK(&comp->work, compat_irq_work);

	if (!comp->wq) {
		snprintf(comp->wq_name, sizeof(comp->wq_name),
			 "compirq/%u-%s", irq, name);
		comp->wq = create_singlethread_workqueue(comp->wq_name);
		if (!comp->wq) {
			printk(KERN_ERR "Failed to create compat-threaded-IRQ workqueue %s\n",
			       comp->wq_name);
			return -ENOMEM;
		}
	}
	return request_irq(irq, compat_irq_dispatcher, flags, name, comp);
}

static inline
void compat_free_threaded_irq(struct compat_threaded_irq *comp)
{
	free_irq(comp->irq, comp);
}

static inline
void compat_destroy_threaded_irq(struct compat_threaded_irq *comp)
{
	if (comp->wq)
		destroy_workqueue(comp->wq);
	comp->wq = NULL;
}

static inline
void compat_synchronize_threaded_irq(struct compat_threaded_irq *comp)
{
	synchronize_irq(comp->irq);
	cancel_work_sync(&comp->work);
}
#endif

#endif /* __BACKPORT_LINUX_INTERRUPT_H */
