#ifndef __BACKPORT_LINUX_NET_H
#define __BACKPORT_LINUX_NET_H
#include_next <linux/net.h>

/* This backports:
 *
 * commit 2033e9bf06f07e049bbc77e9452856df846714cc
 * Author: Neil Horman <nhorman@tuxdriver.com>
 * Date:   Tue May 29 09:30:40 2012 +0000
 *
 *     net: add MODULE_ALIAS_NET_PF_PROTO_NAME
 */
#ifndef MODULE_ALIAS_NET_PF_PROTO_NAME
#define MODULE_ALIAS_NET_PF_PROTO_NAME(pf, proto, name) \
	MODULE_ALIAS("net-pf-" __stringify(pf) "-proto-" __stringify(proto) \
		     name)
#endif

#ifndef net_ratelimited_function
#define net_ratelimited_function(function, ...)			\
do {								\
	if (net_ratelimit())					\
		function(__VA_ARGS__);				\
} while (0)

#define net_emerg_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_emerg, fmt, ##__VA_ARGS__)
#define net_alert_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_alert, fmt, ##__VA_ARGS__)
#define net_crit_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_crit, fmt, ##__VA_ARGS__)
#define net_err_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_err, fmt, ##__VA_ARGS__)
#define net_notice_ratelimited(fmt, ...)			\
	net_ratelimited_function(pr_notice, fmt, ##__VA_ARGS__)
#define net_warn_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_warn, fmt, ##__VA_ARGS__)
#define net_info_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_info, fmt, ##__VA_ARGS__)
#define net_dbg_ratelimited(fmt, ...)				\
	net_ratelimited_function(pr_debug, fmt, ##__VA_ARGS__)
#endif

#endif /* __BACKPORT_LINUX_NET_H */
