#ifndef __BACKPORT_LIST_H
#define __BACKPORT_LIST_H
#include_next <linux/list.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
/**
 * backport:
 *
 * commit 0bbacca7c3911451cea923b0ad6389d58e3d9ce9
 * Author: Sasha Levin <sasha.levin@oracle.com>
 * Date:   Thu Feb 7 12:32:18 2013 +1100
 *
 *     hlist: drop the node parameter from iterators
 */
#include <backport/magic.h>

#undef hlist_entry_safe
#define hlist_entry_safe(ptr, type, member) \
	(ptr) ? hlist_entry(ptr, type, member) : NULL

#define hlist_for_each_entry4(tpos, pos, head, member)			\
	for (pos = (head)->first;					\
	     pos &&							\
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;});\
	     pos = pos->next)

#define hlist_for_each_entry_safe5(tpos, pos, n, head, member)		\
	for (pos = (head)->first;					\
	     pos && ({ n = pos->next; 1; }) &&				\
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;});\
	     pos = n)

#define hlist_for_each_entry3(pos, head, member)				\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);	\
	     pos;								\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlist_for_each_entry_safe4(pos, n, head, member) 			\
	for (pos = hlist_entry_safe((head)->first, typeof(*pos), member);	\
	     pos && ({ n = pos->member.next; 1; });				\
	     pos = hlist_entry_safe(n, typeof(*pos), member))

#undef hlist_for_each_entry
#define hlist_for_each_entry(...) \
	macro_dispatcher(hlist_for_each_entry, __VA_ARGS__)(__VA_ARGS__)
#undef hlist_for_each_entry_safe
#define hlist_for_each_entry_safe(...) \
	macro_dispatcher(hlist_for_each_entry_safe, __VA_ARGS__)(__VA_ARGS__)

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
static inline void __list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	struct list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

static inline void list_cut_position(struct list_head *list,
		struct list_head *head, struct list_head *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __compat_list_splice_new_27(const struct list_head *list,
				 struct list_head *prev,
				 struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

static inline void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__compat_list_splice_new_27(list, head->prev, head);
}

static inline void list_splice_tail_init(struct list_head *list,
					 struct list_head *head)
{
	if (!list_empty(list)) {
		__compat_list_splice_new_27(list, head->prev, head);
		INIT_LIST_HEAD(list);
	}
}
#endif

#ifndef list_first_entry_or_null
/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)
#endif /* list_first_entry_or_null */

#endif /* __BACKPORT_LIST_H */
