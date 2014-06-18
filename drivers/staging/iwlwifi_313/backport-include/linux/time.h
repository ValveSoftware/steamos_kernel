#ifndef __BACKPORT_LINUX_TIME_H
#define __BACKPORT_LINUX_TIME_H
#include_next <linux/time.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
/*
 * Similar to the struct tm in userspace <time.h>, but it needs to be here so
 * that the kernel source is self contained.
 */
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	long tm_year;
	int tm_wday;
	int tm_yday;
};

#define time_to_tm LINUX_BACKPORT(time_to_tm)
void time_to_tm(time_t totalsecs, int offset, struct tm *result);

#endif /* < 2.6.32 */

#endif /* __BACKPORT_LINUX_TIME_H */
