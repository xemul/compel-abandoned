#ifndef __ASM_LIBCOMPEL_REGS_H__
#define __ASM_LIBCOMPEL_REGS_H__

#include "asm-generic/int.h"

typedef struct {
	unsigned long	r15;
	unsigned long	r14;
	unsigned long	r13;
	unsigned long	r12;
	unsigned long	bp;
	unsigned long	bx;
	unsigned long	r11;
	unsigned long	r10;
	unsigned long	r9;
	unsigned long	r8;
	unsigned long	ax;
	unsigned long	cx;
	unsigned long	dx;
	unsigned long	si;
	unsigned long	di;
	unsigned long	orig_ax;
	unsigned long	ip;
	unsigned long	cs;
	unsigned long	flags;
	unsigned long	sp;
	unsigned long	ss;
	unsigned long	fs_base;
	unsigned long	gs_base;
	unsigned long	ds;
	unsigned long	es;
	unsigned long	fs;
	unsigned long	gs;
} user_regs_struct_t;

typedef struct {
	unsigned short	cwd;
	unsigned short	swd;
	unsigned short	twd;		/* Note this is not the same as the 32bit/x87/FSAVE twd */
	unsigned short	fop;
	u64		rip;
	u64		rdp;
	u32		mxcsr;
	u32		mxcsr_mask;
	u32		st_space[32];	/* 8*16 bytes for each FP-reg = 128 bytes */
	u32		xmm_space[64];	/* 16*16 bytes for each XMM-reg = 256 bytes */
	u32		padding[24];
} user_fpregs_struct_t;

#define USER_REGS_IP(r)	((r)->ip)
#define USER_SYSCALL_REG(r) ((r)->orig_ax)
#define USER_REGS_RET(r) ((r)->ax)

#endif /* __ASM_LIBCOMPEL_REGS_H__ */
