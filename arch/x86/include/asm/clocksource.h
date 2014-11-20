/* x86-specific clocksource additions */

#ifndef _ASM_X86_CLOCKSOURCE_H
#define _ASM_X86_CLOCKSOURCE_H

// this would stop the gettimeofday 32 bit VDSO working on x86-64
// see: arch/x86/vdso/vdso32/vclock_gettime.c
// #ifdef CONFIG_X86_64

#define VCLOCK_NONE 0  /* No vDSO clock available.	*/
#define VCLOCK_TSC  1  /* vDSO should use vread_tsc.	*/
#define VCLOCK_HPET 2  /* vDSO should use vread_hpet.	*/
#define VCLOCK_PVCLOCK 3 /* vDSO should use vread_pvclock. */

struct arch_clocksource_data {
	int vclock_mode;
};

// #endif /* CONFIG_X86_64 */

#endif /* _ASM_X86_CLOCKSOURCE_H */
