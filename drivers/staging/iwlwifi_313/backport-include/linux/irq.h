#ifndef __BACKPORT_LINUX_IRQ_H
#define __BACKPORT_LINUX_IRQ_H
#include_next <linux/irq.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
static inline int irq_set_chip(unsigned int irq, struct irq_chip *chip)
{
	return set_irq_chip(irq, chip);
}
static inline int irq_set_handler_data(unsigned int irq, void *data)
{
	return set_irq_data(irq, data);
}
static inline int irq_set_chip_data(unsigned int irq, void *data)
{
	return set_irq_chip_data(irq, data);
}
#ifndef irq_set_irq_type
static inline int irq_set_irq_type(unsigned int irq, unsigned int type)
{
	return set_irq_type(irq, type);
}
#endif
static inline int irq_set_msi_desc(unsigned int irq, struct msi_desc *entry)
{
	return set_irq_msi(irq, entry);
}
static inline struct irq_chip *irq_get_chip(unsigned int irq)
{
	return get_irq_chip(irq);
}
static inline void *irq_get_chip_data(unsigned int irq)
{
	return get_irq_chip_data(irq);
}
static inline void *irq_get_handler_data(unsigned int irq)
{
	return get_irq_data(irq);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static inline void *irq_data_get_irq_handler_data(struct irq_data *d)
{
	return irq_data_get_irq_data(d);
}
#endif

static inline struct msi_desc *irq_get_msi_desc(unsigned int irq)
{
	return get_irq_msi(irq);
}

static inline void irq_set_noprobe(unsigned int irq)
{
	set_irq_noprobe(irq);
}
static inline void irq_set_probe(unsigned int irq)
{
	set_irq_probe(irq);
}
#endif

/* This is really in irqdesc.h, but nothing includes that directly */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39) && \
    LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static inline struct irq_chip *irq_desc_get_chip(struct irq_desc *desc)
{
	return get_irq_desc_chip(desc);
}
static inline void *irq_desc_get_handler_data(struct irq_desc *desc)
{
	return get_irq_desc_data(desc);
}
static inline void *irq_desc_get_chip_data(struct irq_desc *desc)
{
	return get_irq_desc_chip_data(desc);
}
static inline struct msi_desc *irq_desc_get_msi_desc(struct irq_desc *desc)
{
	return get_irq_desc_msi(desc);
}
#endif

#endif /* __BACKPORT_LINUX_IRQ_H */
