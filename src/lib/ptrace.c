#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <elf.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include "compiler.h"
#include "ptrace.h"
#include "log.h"

int unseize_task(pid_t pid, int state)
{
	pr_debug("\tUnseizing %d into %d\n", pid, state);

	if (state == TASK_DEAD)
		kill(pid, SIGKILL);
	else if (state == TASK_STOPPED)
		kill(pid, SIGSTOP);
	else if (state == TASK_ALIVE)
		/* do nothing */ ;
	else
		pr_err("Unknown final state %d\n", state);

	return ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

/*
 * This routine seizes task putting it into a special
 * state where we can manipulate the task via ptrace
 * interface, and finally we can detach ptrace out of
 * of it so the task would not know if it was saddled
 * up with someone else.
 */
int seize_task(pid_t pid)
{
	int status, ret;
	siginfo_t si;

	ret = ptrace(PTRACE_SEIZE, pid, NULL, 0);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't seize task", pid);
		return -1;
	}

	ret = ptrace(PTRACE_INTERRUPT, pid, NULL, NULL);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't interrupt task", pid);
		goto err;
	}

try_again:
	ret = wait4(pid, &status, __WALL, NULL);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't wait task", pid);
		goto err;
	}

	if (ret != pid) {
		pr_err("SEIZE %d: wrong task attached (%d)\n", pid, ret);
		goto err;
	}

	if (!WIFSTOPPED(status)) {
		pr_err("SEIZE %d: task not stopped after seize\n", pid);
		goto err;
	}

	ret = ptrace(PTRACE_GETSIGINFO, pid, NULL, &si);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't read signfo", pid);
		goto err;
	}

	if (SI_EVENT(si.si_code) != PTRACE_EVENT_STOP) {
		/*
		 * Kernel notifies us about the task being seized received some
		 * event other than the STOP, i.e. -- a signal. Let the task
		 * handle one and repeat.
		 */

		if (ptrace(PTRACE_CONT, pid, NULL, (void *)(unsigned long)si.si_signo)) {
			pr_perror("SEIZE %d: can't continue signal handling. Aborting", pid);
			goto err;
		}

		goto try_again;
	}

	if (si.si_signo == SIGTRAP)
		return TASK_ALIVE;
	else if (si.si_signo == SIGSTOP) {
		/*
		 * PTRACE_SEIZE doesn't affect signal or group stop state.
		 * Currently ptrace reported that task is in stopped state.
		 * We need to start task again, and it will be trapped
		 * immediately, because we sent PTRACE_INTERRUPT to it.
		 */
		ret = ptrace(PTRACE_CONT, pid, 0, 0);
		if (ret) {
			pr_perror("SEIZE %d: unable to start process", pid);
			goto err;
		}

		ret = wait4(pid, &status, __WALL, NULL);
		if (ret < 0) {
			pr_perror("SEIZE %d: can't wait task", pid);
			goto err;
		}

		if (ret != pid) {
			pr_err("SEIZE %d: wrong task attached (%d)\n", pid, ret);
			goto err;
		}

		if (!WIFSTOPPED(status)) {
			pr_err("SEIZE %d: task not stopped after seize\n", pid);
			goto err;
		}

		return TASK_STOPPED;
	}

	pr_err("SEIZE %d: unsupported stop signal %d\n", pid, si.si_signo);
err:
	unseize_task(pid, TASK_STOPPED);
	return -1;
}

int ptrace_peek_area(pid_t pid, void *dst, void *addr, unsigned long bytes)
{
	unsigned long w;
	if (bytes & (sizeof(long) - 1))
		return -1;
	for (w = 0; w < bytes / sizeof(long); w++) {
		unsigned long *d = dst, *a = addr;
		d[w] = ptrace(PTRACE_PEEKDATA, pid, a + w, NULL);
		if (d[w] == -1U && errno)
			goto err;
	}
	return 0;
err:
	return -2;
}

int ptrace_poke_area(pid_t pid, void *src, void *addr, unsigned long bytes)
{
	unsigned long w;
	if (bytes & (sizeof(long) - 1))
		return -1;
	for (w = 0; w < bytes / sizeof(long); w++) {
		unsigned long *s = src, *a = addr;
		if (ptrace(PTRACE_POKEDATA, pid, a + w, s[w]))
			goto err;
	}
	return 0;
err:
	return -2;
}

/* don't swap big space, it might overflow the stack */
int ptrace_swap_area(pid_t pid, void *dst, void *src, unsigned long bytes)
{
	void *t = alloca(bytes);

	if (ptrace_peek_area(pid, t, dst, bytes))
		return -1;

	if (ptrace_poke_area(pid, src, dst, bytes)) {
		if (ptrace_poke_area(pid, t, dst, bytes))
			return -2;
		return -1;
	}

	memcpy(src, t, bytes);
	return 0;
}

int ptrace_get_regs(int pid, user_regs_struct_t *regs)
{
	struct iovec iov = {
		.iov_base	= regs,
		.iov_len	= sizeof(*regs),
	};
	return ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
}

int ptrace_set_regs(int pid, user_regs_struct_t *regs)
{
	struct iovec iov = {
		.iov_base	= regs,
		.iov_len	= sizeof(*regs),
	};
	return ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
}

int ptrace_get_thread_ctx(pid_t pid, thread_ctx_t *ctx)
{
	if (ptrace(PTRACE_GETSIGMASK, pid, sizeof(ctx->sigmask), &ctx->sigmask)) {
		pr_perror("can't get signal blocking mask for %d", pid);
		return -1;
	}

	if (ptrace_get_regs(pid, &ctx->regs)) {
		pr_perror("Can't obtain registers (pid: %d)", pid);
		return -1;
	}

	return ptrace_get_fpu(pid, &ctx->fpu);
}

int ptrace_set_thread_ctx(pid_t pid, thread_ctx_t *ctx)
{
	int ret = 0;

	if (ptrace_set_regs(pid, &ctx->regs)) {
		pr_perror("Can't restore registers (pid: %d)", pid);
		ret = -1;
	}

	if (ptrace(PTRACE_SETSIGMASK, pid, sizeof(ctx->sigmask), &ctx->sigmask)) {
		pr_perror("Can't block signals");
		ret = -1;
	}

	if (!ret)
		ret = ptrace_set_fpu(pid, &ctx->fpu);
	return ret;
}
