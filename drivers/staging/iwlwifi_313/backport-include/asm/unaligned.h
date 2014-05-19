#ifndef __BACKPORT_ASM_UNALIGNED_H
#define __BACKPORT_ASM_UNALIGNED_H
#include_next <asm/unaligned.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
/*
 * 2.6.26 added its own unaligned API which the
 * new drivers can use. Lets port it here by including it in older
 * kernels and also deal with the architecture handling here.
 */
#ifdef CONFIG_ALPHA

#include <linux/unaligned/be_struct.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* alpha */
#ifdef CONFIG_ARM

/* arm */
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* arm */
#ifdef CONFIG_AVR32

/*
 * AVR32 can handle some unaligned accesses, depending on the
 * implementation.  The AVR32 AP implementation can handle unaligned
 * words, but halfwords must be halfword-aligned, and doublewords must
 * be word-aligned.
 *
 * However, swapped word loads must be word-aligned so we can't
 * optimize word loads in general.
 */

#include <linux/unaligned/be_struct.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif
#ifdef CONFIG_BLACKFIN

#include <linux/unaligned/le_struct.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* blackfin */
#ifdef CONFIG_CRIS

/*
 * CRIS can do unaligned accesses itself.
 */
#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#endif /* cris */
#ifdef CONFIG_FRV

#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* frv */
#ifdef CONFIG_H8300

#include <linux/unaligned/be_memmove.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* h8300 */
#ifdef  CONFIG_IA64

#include <linux/unaligned/le_struct.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* ia64 */
#ifdef CONFIG_M32R

#if defined(__LITTLE_ENDIAN__)
# include <linux/unaligned/le_memmove.h>
# include <linux/unaligned/be_byteshift.h>
# include <linux/unaligned/generic.h>
#else
# include <linux/unaligned/be_memmove.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>
#endif

#endif /* m32r */
#ifdef CONFIG_M68K /* this handles both m68k and m68knommu */

#ifdef CONFIG_COLDFIRE
#include <linux/unaligned/be_struct.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>
#else

/*
 * The m68k can do unaligned accesses itself.
 */
#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>
#endif

#endif /* m68k and m68knommu */
#ifdef CONFIG_MIPS

#if defined(__MIPSEB__)
# include <linux/unaligned/be_struct.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>
# define get_unaligned  __get_unaligned_be
# define put_unaligned  __put_unaligned_be
#elif defined(__MIPSEL__)
# include <linux/unaligned/le_struct.h>
# include <linux/unaligned/be_byteshift.h>
# include <linux/unaligned/generic.h>
#endif

#endif /* mips */
#ifdef CONFIG_MN10300

#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#endif /* mn10300 */
#ifdef CONFIG_PARISC

#include <linux/unaligned/be_struct.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* parisc */
#ifdef CONFIG_PPC
/*
 * The PowerPC can do unaligned accesses itself in big endian mode.
 */
#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#endif /* ppc */
#ifdef CONFIG_S390

/*
 * The S390 can do unaligned accesses itself.
 */
#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#endif /* s390 */
#ifdef CONFIG_SUPERH

/* SH can't handle unaligned accesses. */
#ifdef __LITTLE_ENDIAN__
# include <linux/unaligned/le_struct.h>
# include <linux/unaligned/be_byteshift.h>
# include <linux/unaligned/generic.h>
#else
# include <linux/unaligned/be_struct.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>
#endif

#endif /* sh - SUPERH */
#ifdef CONFIG_SPARC

/* sparc and sparc64 */
#include <linux/unaligned/be_struct.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif  /* sparc */
#ifdef CONFIG_UML

#include "asm/arch/unaligned.h"

#endif /* um - uml */
#ifdef CONFIG_V850

#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/generic.h>

#endif /* v850 */
#ifdef CONFIG_X86
/*
 * The x86 can do unaligned accesses itself.
 */
#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#endif /* x86 */
#ifdef CONFIG_XTENSA

#ifdef __XTENSA_EL__
# include <linux/unaligned/le_memmove.h>
# include <linux/unaligned/be_byteshift.h>
# include <linux/unaligned/generic.h>
#elif defined(__XTENSA_EB__)
# include <linux/unaligned/be_memmove.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>
#else
# error processor byte order undefined!
#endif

#endif /* xtensa */
#endif /* < 2.6.26 */

#endif /* __BACKPORT_ASM_UNALIGNED_H */
