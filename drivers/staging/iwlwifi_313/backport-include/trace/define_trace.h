#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32))
#include_next <trace/define_trace.h>
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)) */
