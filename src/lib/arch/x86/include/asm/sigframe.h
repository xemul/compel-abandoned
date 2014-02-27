#ifndef __ASM_LIBCOMPEL_SIGFRAME_H__
#define __ASM_LIBCOMPEL_SIGFRAME_H__

#include "asm/regs.h"
#include "asm/fpu.h"
#include "asm/sigset.h"

#include "compiler.h"

#define SIGFRAME_OFFSET 8

/* sigframe should be aligned on 64 byte for x86 and 8 bytes for arm */
#define SIGFRAME_SIZE		ALIGN(sizeof(struct rt_sigframe) + SIGFRAME_OFFSET, 64)

#ifndef __ARCH_SI_PREAMBLE_SIZE
#define __ARCH_SI_PREAMBLE_SIZE	(3 * sizeof(int))
#endif

#define SI_MAX_SIZE	128
#ifndef SI_PAD_SIZE
#define SI_PAD_SIZE	((SI_MAX_SIZE - __ARCH_SI_PREAMBLE_SIZE) / sizeof(int))
#endif

typedef struct rt_siginfo {
	int	si_signo;
	int	si_errno;
	int	si_code;
	int	_pad[SI_PAD_SIZE];
} rt_siginfo_t;

struct rt_sigcontext {
	unsigned long		r8;
	unsigned long		r9;
	unsigned long		r10;
	unsigned long		r11;
	unsigned long		r12;
	unsigned long		r13;
	unsigned long		r14;
	unsigned long		r15;
	unsigned long		rdi;
	unsigned long		rsi;
	unsigned long		rbp;
	unsigned long		rbx;
	unsigned long		rdx;
	unsigned long		rax;
	unsigned long		rcx;
	unsigned long		rsp;
	unsigned long		rip;
	unsigned long		eflags;
	unsigned short		cs;
	unsigned short		gs;
	unsigned short		fs;
	unsigned short		__pad0;
	unsigned long		err;
	unsigned long		trapno;
	unsigned long		oldmask;
	unsigned long		cr2;
	void			*fpstate;
	unsigned long		reserved1[8];
};

typedef struct rt_sigaltstack {
	void	*ss_sp;
	int	ss_flags;
	size_t	ss_size;
} rt_stack_t;

struct rt_ucontext {
	unsigned long		uc_flags;
	struct rt_ucontext	*uc_link;
	rt_stack_t		uc_stack;
	struct rt_sigcontext	uc_mcontext;
	k_rtsigset_t		uc_sigmask;	/* mask last for extensibility */
	int                     __unused[32 - (sizeof (k_rtsigset_t) / sizeof (int))];
	unsigned long           uc_regspace[128] __attribute__((__aligned__(8)));
};

typedef struct rt_sigframe {
	char			*pretcode;
	struct rt_ucontext	uc;
	struct rt_siginfo	info;

	fpu_state_t		fpu_state;
} rt_sigframe_t;

#endif /* __ASM_LIBCOMPEL_SIGFRAME_H__ */
