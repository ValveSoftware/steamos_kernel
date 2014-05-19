#ifndef __BACKPORT_NET_IW_HANDLER_H
#define __BACKPORT_NET_IW_HANDLER_H
#include_next <net/iw_handler.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#define wireless_send_event(a, b, c, d) wireless_send_event(a, b, c, (char * ) d)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define iwe_stream_add_value(info, event, value, ends, iwe, event_len) iwe_stream_add_value(event, value, ends, iwe, event_len)
#define iwe_stream_add_point(info, stream, ends, iwe, extra) iwe_stream_add_point(stream, ends, iwe, extra)
#define iwe_stream_add_event(info, stream, ends, iwe, event_len) iwe_stream_add_event(stream, ends, iwe, event_len)

#define IW_REQUEST_FLAG_COMPAT	0x0001	/* Compat ioctl call */

static inline int iwe_stream_lcp_len(struct iw_request_info *info)
{
#ifdef CONFIG_COMPAT
	if (info->flags & IW_REQUEST_FLAG_COMPAT)
		return IW_EV_COMPAT_LCP_LEN;
#endif
	return IW_EV_LCP_LEN;
}
#endif

#endif /* __BACKPORT_NET_IW_HANDLER_H */
