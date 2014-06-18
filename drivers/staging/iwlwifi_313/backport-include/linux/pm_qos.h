#ifndef _COMPAT_LINUX_PM_QOS_H
#define _COMPAT_LINUX_PM_QOS_H 1

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
#include_next <linux/pm_qos.h>
#else
#include <linux/pm_qos_params.h>
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)) */

#ifndef PM_QOS_DEFAULT_VALUE
#define PM_QOS_DEFAULT_VALUE -1
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
struct pm_qos_request_list {
	u32 qos;
	void *request;
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))

#define pm_qos_add_request(_req, _class, _value) do {			\
	(_req)->request = #_req;					\
	(_req)->qos = _class;						\
	pm_qos_add_requirement((_class), (_req)->request, (_value));	\
    } while(0)

#define pm_qos_update_request(_req, _value)				\
	pm_qos_update_requirement((_req)->qos, (_req)->request, (_value))

#define pm_qos_remove_request(_req)					\
	pm_qos_remove_requirement((_req)->qos, (_req)->request)

#else

#define pm_qos_add_request(_req, _class, _value) do {			\
	(_req)->request = pm_qos_add_request((_class), (_value));	\
    } while (0)

#define pm_qos_update_request(_req, _value)				\
	pm_qos_update_request((_req)->request, (_value))

#define pm_qos_remove_request(_req)					\
	pm_qos_remove_request((_req)->request)

#endif /* < 2.6.35 */
#endif /* < 2.6.36 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#define pm_qos_request(_qos) pm_qos_requirement(_qos)
#endif

#endif	/* _COMPAT_LINUX_PM_QOS_H */
