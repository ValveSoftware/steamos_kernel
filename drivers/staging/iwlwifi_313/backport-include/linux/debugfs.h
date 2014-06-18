#ifndef __BACKPORT_LINUX_DEBUGFS_H
#define __BACKPORT_LINUX_DEBUGFS_H
#include_next <linux/debugfs.h>
#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define debugfs_remove_recursive LINUX_BACKPORT(debugfs_remove_recursive)

#if defined(CONFIG_DEBUG_FS)
void debugfs_remove_recursive(struct dentry *dentry);
#else
static inline void debugfs_remove_recursive(struct dentry *dentry)
{ }
#endif
#endif /* < 2.6.27 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#if RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(6,4)
static inline struct dentry *debugfs_create_x64(const char *name, umode_t mode,
						struct dentry *parent,
						u64 *value)
{
	return debugfs_create_u64(name, mode, parent, value);
}
#endif
#endif

#endif /* __BACKPORT_LINUX_DEBUGFS_H */
