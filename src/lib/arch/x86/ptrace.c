#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include "compiler.h"
#include "ptrace.h"
#include "log.h"

#ifndef NT_X86_XSTATE
# define NT_X86_XSTATE	0x202		/* x86 extended state using xsave */
#endif

bool task_in_compat_mode(pid_t pid)
{
	unsigned long cs, ds;

	errno = 0;
	cs = ptrace(PTRACE_PEEKUSER, pid, offsetof(user_regs_struct_t, cs), 0);
	if (errno != 0) {
		pr_perror("Can't get CS register for %d", pid);
		return -1;
	}

	errno = 0;
	ds = ptrace(PTRACE_PEEKUSER, pid, offsetof(user_regs_struct_t, ds), 0);
	if (errno != 0) {
		pr_perror("Can't get DS register for %d", pid);
		return -1;
	}

	/* It's x86-32 or x32 */
	return cs != 0x33 || ds == 0x2b;
}

int ptrace_get_fpu(pid_t pid, fpu_state_t *fpu)
{
	struct iovec iov = {
		.iov_base	= &fpu->xsave,
		.iov_len	= sizeof(fpu->xsave),
	};

	if (ptrace(PTRACE_GETREGSET, pid, (unsigned int)NT_X86_XSTATE, &iov) < 0) {
		if (errno != ENODEV)
			goto fpuerr;
		else
			if (ptrace(PTRACE_GETFPREGS, pid, NULL, &fpu->xsave) < 0)
				goto fpuerr;
			fpu->has_xsave = false;
	} else
		fpu->has_xsave = true;
	return 0;

fpuerr:
	pr_perror("Can't obtain FPU registers for %d", pid);
	return -1;
}

int ptrace_set_fpu(pid_t pid, fpu_state_t *fpu)
{
	struct iovec iov = {
		.iov_base	= &fpu->xsave,
		.iov_len	= sizeof(fpu->xsave),
	};

	if (fpu->has_xsave) {
		if (ptrace(PTRACE_SETREGSET, pid, (unsigned int)NT_X86_XSTATE, &iov) < 0)
			goto fpuerr;
	} else {
		if (ptrace(PTRACE_SETFPREGS, pid, NULL, &fpu->xsave) < 0)
			goto fpuerr;
	}
	return 0;

fpuerr:
	pr_perror("Can't set FPU registers for %d", pid);
	return -1;
}
