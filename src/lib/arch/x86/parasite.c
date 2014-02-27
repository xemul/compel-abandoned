#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <syscall.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <elf.h>

#include "asm/processor-flags.h"
#include "asm/sigframe.h"
#include "asm/prologue.h"

#include "compiler.h"
#include "parasite.h"
#include "ptrace.h"
#include "xmalloc.h"
#include "log.h"

#define ERESTARTSYS		512
#define ERESTARTNOINTR		513
#define ERESTARTNOHAND		514
#define ERESTART_RESTARTBLOCK	516

int parasite_syscall_seized(parasite_ctl_t *ctl,
			    int nr, unsigned long *ret,
			    unsigned long arg1,
			    unsigned long arg2,
			    unsigned long arg3,
			    unsigned long arg4,
			    unsigned long arg5,
			    unsigned long arg6)
{
	user_regs_struct_t regs = ctl->thread_ctx_orig.regs;
	int err;

	regs.ax  = (unsigned long)nr;
	regs.di  = arg1;
	regs.si  = arg2;
	regs.dx  = arg3;
	regs.r10 = arg4;
	regs.r8  = arg5;
	regs.r9  = arg6;

	err = parasite_execute_syscall_trap(ctl, &regs);

	*ret = regs.ax;
	return err;
}

void parasite_setup_regs(unsigned long new_ip, void *stack, user_regs_struct_t *regs)
{
	regs->ip = new_ip;
	if (stack)
		regs->sp = (unsigned long)stack;

	/* Avoid end of syscall processing */
	regs->orig_ax = -1;

	/* Make sure flags are in known state */
	regs->flags &= ~(X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF);
}

void parasite_fixup_thread_ctx(thread_ctx_t *ctx)
{
	if ((int)ctx->regs.orig_ax >= 0) {
		/* Restart the system call */
		switch ((int)ctx->regs.ax) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			ctx->regs.ax  = ctx->regs.orig_ax;
			ctx->regs.ip -= 2;
			break;
		case -ERESTART_RESTARTBLOCK:
			ctx->regs.ax  = __NR_restart_syscall;
			ctx->regs.ip -= 2;
			break;
		}
	}
}

void parasite_prepare_sigframe(parasite_ctl_t *c, unsigned long sigframe_at)
{
	rt_sigframe_t *sigframe;

	/*
	 * Construct sigframe.
	 *
	 * Note: SAS get constructed in std plugin, because
	 * it can't be fetched here.
	 */
	sigframe = (void *)sigframe_at;
	memzero(sigframe, sizeof(*sigframe));

	sigframe->uc.uc_mcontext.r8	= c->thread_ctx_orig.regs.r8;
	sigframe->uc.uc_mcontext.r9	= c->thread_ctx_orig.regs.r9;
	sigframe->uc.uc_mcontext.r10	= c->thread_ctx_orig.regs.r10;
	sigframe->uc.uc_mcontext.r11	= c->thread_ctx_orig.regs.r11;
	sigframe->uc.uc_mcontext.r12	= c->thread_ctx_orig.regs.r12;
	sigframe->uc.uc_mcontext.r13	= c->thread_ctx_orig.regs.r13;
	sigframe->uc.uc_mcontext.r14	= c->thread_ctx_orig.regs.r14;
	sigframe->uc.uc_mcontext.r15	= c->thread_ctx_orig.regs.r15;
	sigframe->uc.uc_mcontext.rdi	= c->thread_ctx_orig.regs.di;
	sigframe->uc.uc_mcontext.rsi	= c->thread_ctx_orig.regs.si;
	sigframe->uc.uc_mcontext.rbp	= c->thread_ctx_orig.regs.bp;
	sigframe->uc.uc_mcontext.rbx	= c->thread_ctx_orig.regs.bx;
	sigframe->uc.uc_mcontext.rdx	= c->thread_ctx_orig.regs.dx;
	sigframe->uc.uc_mcontext.rax	= c->thread_ctx_orig.regs.ax;
	sigframe->uc.uc_mcontext.rcx	= c->thread_ctx_orig.regs.cx;
	sigframe->uc.uc_mcontext.rsp	= c->thread_ctx_orig.regs.sp;
	sigframe->uc.uc_mcontext.rip	= c->thread_ctx_orig.regs.ip;
	sigframe->uc.uc_mcontext.eflags	= c->thread_ctx_orig.regs.flags;
	sigframe->uc.uc_mcontext.cs	= c->thread_ctx_orig.regs.cs;
	sigframe->uc.uc_mcontext.gs	= c->thread_ctx_orig.regs.gs;
	sigframe->uc.uc_mcontext.fs	= c->thread_ctx_orig.regs.fs;

	sigframe->uc.uc_sigmask		= c->thread_ctx_orig.sigmask;

	sigframe->fpu_state		= c->thread_ctx_orig.fpu;
}
