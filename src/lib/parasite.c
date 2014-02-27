#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "compiler.h"
#include "parasite.h"
#include "ptrace.h"
#include "xmalloc.h"
#include "bug.h"
#include "elf.h"
#include "log.h"
#include "err.h"

#include "asm/sigframe.h"
#include "asm/prologue.h"

static const char code_syscall[ASM_RAW_SYSCALL_SIZE] = ASM_RAW_SYSCALL;

static int parasite_find_vma_for_syscall(pid_t pid, unsigned long *start, unsigned long *end)
{
	char buf[128];
	char path[32];
	int ret = -1;
	FILE *f;

	char r, w, x, s;
	int dev_maj, dev_min;
	unsigned long from, to, pgoff, ino;
	int n;


	snprintf(buf, sizeof(buf), "/proc/%d/maps", pid);
	f = fopen(buf, "r");
	if (!f) {
		pr_perror("Can't open %s", buf);
		return -1;
	}

	while (fgets(buf, sizeof(buf), f)) {
		memzero(path, sizeof(path));

		n = sscanf(buf, "%lx-%lx %c%c%c%c %lx %x:%x %lu %9s",
			   &from, &to, &r, &w, &x, &s, &pgoff, &dev_maj,
			   &dev_min, &ino, path);
		if (n < 10) {
			pr_err("Can't parse `%s'\n", buf);
			goto err;
		}

		/*
		 * We need at least one executable zone which we will
		 * poke to inject mmap() syscall.
		 */
		if (x == 'x' && ((to - from) > ASM_RAW_SYSCALL_SIZE) &&
		    !parasite_is_service_vma(path) &&
		    from < TASK_SIZE) {
			ret = 0;
			*start = from;
			*end = to;
			break;
		}
	}
err:
	fclose(f);
	if (ret)
		pr_err("No suitable exec VMA found for %d\n", pid);
	else
		pr_debug("Donor %d vma %lx-%lx\n", pid, *start, *end);
	return ret;
}

static void *parasite_mmap_seized(void *ctl, void *addr, size_t length, int prot,
				  int flags, int fd, off_t offset)
{
	unsigned long map;
	int err;

	err = parasite_syscall_seized(ctl, __NR_mmap, &map,
				      (unsigned long)addr, length, prot, flags, fd, offset);
	if (err < 0 || map > TASK_SIZE)
		map = 0;

	return (void *)map;
}

static int parasite_munmap_seized(void *ctl, void *addr, size_t length)
{
	unsigned long ret;
	int err;

	err = parasite_syscall_seized(ctl, __NR_munmap, &ret,
				      (unsigned long)addr, length,
				      0, 0, 0, 0);
	return (err | (int)ret);
}

static int parasite_run(pid_t pid, int cmd, unsigned long ip, void *stack,
			user_regs_struct_t *regs, thread_ctx_t *octx)
{
	k_rtsigset_t block;

	ksigfillset(&block);
	if (ptrace(PTRACE_SETSIGMASK, pid, sizeof(k_rtsigset_t), &block)) {
		pr_perror("Can't block signals for %d", pid);
		goto err_sig;
	}

	parasite_setup_regs(ip, stack, regs);
	if (ptrace_set_regs(pid, regs)) {
		pr_perror("Can't set registers for %d", pid);
		goto err_regs;
	}

	if (ptrace(cmd, pid, NULL, NULL)) {
		pr_perror("Can't run parasite at %d", pid);
		goto err_cont;
	}

	return 0;

err_cont:
	if (ptrace_set_regs(pid, &octx->regs))
		pr_perror("Can't restore regs for %d", pid);
err_regs:
	if (ptrace(PTRACE_SETSIGMASK, pid, sizeof(k_rtsigset_t), &octx->sigmask))
		pr_perror("Can't restore sigmask for %d", pid);
err_sig:
	return -1;
}

/*
 * Trap tasks on the exit from the specified syscall
 *
 * tasks - number of processes, which should be trapped
 * sys_nr - the required syscall number
 */
static int parasite_stop_on_syscall(int tasks, const int sys_nr)
{
	user_regs_struct_t regs;
	int status, ret;
	pid_t pid;

	/* Stop all threads on the enter point in sys_rt_sigreturn */
	while (tasks) {
		pid = wait4(-1, &status, __WALL, NULL);
		if (pid == -1) {
			pr_perror("wait4 failed");
			return -1;
		}

		if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP) {
			pr_err("Task %d is in unexpected state: %x\n", pid, status);
			return -1;
		}

		pr_debug("Task %d was trapped\n", pid);
		if (!WIFSTOPPED(status)) {
			pr_err("But %d is not stopped %d\n", pid, status);
			return -1;
		}

		ret = ptrace_get_regs(pid, &regs);
		if (ret) {
			pr_perror("Can't get registers for %d", pid);
			return -1;
		}

		pr_debug("Task %d is going to execute the syscall %lx\n", pid, USER_SYSCALL_REG(&regs));
		if (USER_SYSCALL_REG(&regs) == sys_nr) {
			/*
			 * The process is going to execute the required syscall,
			 * the next stop will be on the exit from this syscall
			 */
			ret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
			if (ret) {
				pr_perror("PTRACE_SYSCALL for %d failed", pid);
				return -1;
			}

			pid = wait4(pid, &status, __WALL, NULL);
			if (pid == -1) {
				pr_perror("wait4 failed");
				return -1;
			}

			if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP) {
				pr_err("Task %d is in unexpected state: %x\n", pid, status);
				return -1;
			}

			pr_debug("Task %d was stopped\n", pid);
			tasks--;
			continue;
		}

		ret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		if (ret) {
			pr_perror("PTRACE_SYSCALL for %d failed", pid);
			return -1;
		}
	}

	return 0;
}

/* we run at @regs->ip */
static int parasite_trap(parasite_ctl_t *ctl, pid_t pid, user_regs_struct_t *regs, thread_ctx_t *octx)
{
	int status, ret = -1;
	siginfo_t siginfo;

	if (wait4(pid, &status, __WALL, NULL) != pid) {
		pr_perror("Waited pid mismatch (pid: %d)", pid);
		goto err;
	}

	if (!WIFSTOPPED(status)) {
		pr_err("Task is still running (pid: %d)\n", pid);
		goto err;
	}

	if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo)) {
		pr_perror("Can't get siginfo (pid: %d)", pid);
		goto err;
	}

	if (ptrace_get_regs(pid, regs)) {
		pr_perror("Can't obtain registers (pid: %d)", pid);
			goto err;
	}

	if (WSTOPSIG(status) != SIGTRAP || siginfo.si_code != ARCH_SI_TRAP) {
		pr_debug("** delivering signal %d si_code=%d\n",
			 siginfo.si_signo, siginfo.si_code);

		pr_err("Unexpected %d task interruption, aborting\n", pid);
		goto err;
	}

	/*
	 * We've reached this point if int3 is triggered inside our
	 * parasite code. So we're done.
	 */
	ret = 0;
err:
	if (ptrace_set_thread_ctx(pid, octx))
		ret = -1;
	return ret;
}

int parasite_execute_syscall_trap(parasite_ctl_t *ctl, user_regs_struct_t *regs)
{
	pid_t pid = ctl->pid;
	int err;

	BUILD_BUG_ON(ASM_RAW_SYSCALL_SIZE != BUILTIN_SYSCALL_SIZE);

	/*
	 * Inject syscall instruction and remember original code,
	 * we will need it to restore original program content.
	 */
	memcpy(ctl->code_orig, code_syscall, sizeof(ctl->code_orig));
	if (ptrace_swap_area(pid, (void *)ctl->syscall_ip,
			     (void *)ctl->code_orig, sizeof(ctl->code_orig))) {
		pr_err("Can't inject syscall blob (pid: %d)\n", pid);
		return -1;
	}

	err = parasite_run(pid, PTRACE_CONT, ctl->syscall_ip, 0, regs, &ctl->thread_ctx_orig);
	if (!err)
		err = parasite_trap(ctl, pid, regs, &ctl->thread_ctx_orig);

	if (ptrace_poke_area(pid, (void *)ctl->code_orig,
			     (void *)ctl->syscall_ip, sizeof(ctl->code_orig))) {
		pr_err("Can't restore syscall blob (pid: %d)\n", ctl->pid);
		err = -1;
	}

	return err;
}

/*
 * We need to detect parasite crashes not to hang on socket operations.
 * Since we hold parasite with ptrace, it will receive SIGCHLD if the
 * latter would crash.
 */
static void sigchld_handler(int signal, siginfo_t *siginfo, void *data)
{
	int pid, status;

	pr_err("SIGCHLD catched si_code=%d si_pid=%d si_status=%d\n",
		siginfo->si_code, siginfo->si_pid, siginfo->si_status);

	pid = waitpid(-1, &status, WNOHANG);
	if (pid <= 0)
		return;

	if (WIFEXITED(status))
		pr_err("%d exited with %d unexpectedly\n", pid, WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		pr_err("%d was killed by %d unexpectedly\n", pid, WTERMSIG(status));
	else if (WIFSTOPPED(status))
		pr_err("%d was stopped by %d unexpectedly\n", pid, WSTOPSIG(status));

	exit(1);
}

static int setup_child_handler()
{
	struct sigaction sa = {
		.sa_sigaction	= sigchld_handler,
		.sa_flags	= SA_SIGINFO | SA_RESTART,
	};

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGCHLD);

	if (sigaction(SIGCHLD, &sa, NULL)) {
		pr_perror("Unable to setup SIGCHLD handler");
		return -1;
	}

	return 0;
}

static int restore_child_handler()
{
	struct sigaction sa = {
		.sa_handler	= SIG_DFL,
		.sa_flags	= SA_SIGINFO | SA_RESTART,
	};

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGCHLD);

	if (sigaction(SIGCHLD, &sa, NULL)) {
		pr_perror("Unable to setup SIGCHLD handler");
		return -1;
	}

	return 0;
}

static int parasite_ctl_fini(parasite_ctl_t *ctl)
{
	int ret = 0;

	if (!ctl)
		return 0;

	if (ctl->ctl_sock >= 0)
		close(ctl->ctl_sock);

	if (ctl->ctl_accepted >= 0)
		close(ctl->ctl_accepted);

	if (ctl->remote_map) {
		ret |= parasite_munmap_seized(ctl, ctl->remote_map, ctl->map_length);
		ctl->remote_map = NULL;
	}

	if (ctl->local_map) {
		ret |= munmap(ctl->local_map, ctl->map_length);
		ctl->local_map = NULL;
	}

	ret |= ptrace_set_thread_ctx(ctl->pid, &ctl->thread_ctx_orig);

	memzero(ctl, sizeof(*ctl));
	ctl->ctl_sock = -1;
	ctl->ctl_accepted = -1;

	return ret;
}

static int gen_std_saddr(struct sockaddr_un *saddr, int key)
{
	int sun_len;

#ifndef UNIX_PATH_MAX
# define UNIX_PATH_MAX 108
#endif

	saddr->sun_family = AF_UNIX;
	snprintf(saddr->sun_path, UNIX_PATH_MAX, "X/compel-ctl-%d", key);
	sun_len = SUN_LEN(saddr);
	*saddr->sun_path = '\0';
	return sun_len;
}

static parasite_ctl_t *parasite_ctl_init(pid_t pid, char *blob_path, void *arg_p, unsigned int arg_s)
{
	prologue_init_args_t *args;
	unsigned long start, end;
	load_info_t info = { };
	struct stat plugin_st;
	parasite_ctl_t *ctl;
	int ret = -1, fd;
	char path[128];
	int plugin_fd;
	unsigned long init_args_at;
	unsigned long sigframe_at;

	plugin_fd = open(blob_path, O_RDONLY);
	if (plugin_fd < 0) {
		pr_perror("Can't open %s", blob_path);
		return NULL;
	}

	ctl = xzalloc(sizeof(*ctl));
	if (!ctl) {
		close(plugin_fd);
		return NULL;
	}
	ctl->ctl_sock = -1;
	ctl->ctl_accepted = -1;

	if (fstat(plugin_fd, &plugin_st)) {
		pr_perror("Failed to obtain statistics on %s", blob_path);
		goto err;
	}

	if (parasite_find_vma_for_syscall(pid, &start, &end)) {
		pr_err("Can't infect task %d\n", pid);
		goto err;
	}

	ctl->pid	= pid;
	ctl->syscall_ip = start;

	if (ptrace_get_thread_ctx(pid, &ctl->thread_ctx_orig)) {
		pr_err("Can't fetch thread %d context\n", pid);
		goto err;
	}
	parasite_fixup_thread_ctx(&ctl->thread_ctx_orig);
	ctl->map_length = round_up(plugin_st.st_size + arg_s, PAGE_SIZE);

	ctl->remote_map = parasite_mmap_seized(ctl, NULL, ctl->map_length,
					       PROT_READ | PROT_WRITE | PROT_EXEC,
					       MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (!ctl->remote_map) {
		pr_err("Can't allocate memory for parasite blob (pid: %d)\n", ctl->pid);
		goto err;
	}

	snprintf(path, sizeof(path), "/proc/%d/map_files/%p-%p",
		 ctl->pid, ctl->remote_map, ctl->remote_map + ctl->map_length);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		pr_perror("Can't open %s", path);
		goto err_restore;
	}

	ctl->local_map = mmap(NULL, ctl->map_length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);

	if (ctl->local_map == MAP_FAILED) {
		ctl->local_map = NULL;
		pr_perror("Can't map remote parasite map");
		goto err_restore;
	}

	/*
	 * How executing blob is "packed"
	 *                                                        (x86-64 as example)
	 * +--------------------+                                 +----------------------+
	 * | parasite prologue  | plugin_ip   ==> consists of ==> | call to "main"       |
	 * +--------------------+                                 +        ...           |
	 * | rt-sigframe        | plugin_sigframe                 | syscall rt-sigreturn |
	 * +--------------------+                                 +----------------------+
	 * | blob               |
	 * +--------------------+
	 * | plugin arguments   |
	 * +--------------------+
	 *
	 * This is a final picture, but before that we need to
	 *
	 *  - upload code of the plugin into proper place
	 *  - apply relocations
	 */

	pr_info("Putting exec blob into l:%p -> r:%p\n", ctl->local_map, ctl->remote_map);
	if (read(plugin_fd, ctl->local_map, plugin_st.st_size) != plugin_st.st_size) {
		pr_perror("Failed to read plugin %s code", blob_path);
		goto err;
	}

	if (arg_p)
		memcpy(ctl->local_map + plugin_st.st_size, arg_p, arg_s);
	else
		memzero(ctl->local_map + plugin_st.st_size, arg_s);

	info.len	= plugin_st.st_size;
	info.hdr	= ctl->local_map;
	info.addr_delta	= ctl->remote_map - ctl->local_map;

	pr_debug("Loading plugin %s (size %ld delta %lx)\n",
		 blob_path, info.len, info.addr_delta);

	if (load_elf_plugin(&info)) {
		pr_err("Failed to load plugin %s\n", blob_path);
		goto err;
	}

	ctl->entry_ip = lookup_elf_plugin_remote_symbol(&info, "__export_std_prologue_start");
	sigframe_at = lookup_elf_plugin_local_symbol(&info, "__export_std_prologue_sigframe");
	init_args_at = lookup_elf_plugin_local_symbol(&info, "__export_std_prologue_init_args");

	pr_debug("Parasite code: entry_ip %p sigframe_at %lx init_args_at %lx\n",
		 ctl->entry_ip, sigframe_at, init_args_at);

	if (!ctl->entry_ip || !sigframe_at || !init_args_at) {
		pr_err("Not all symbols needed found, corrupted file?\n");
		goto err;
	}

	parasite_prepare_sigframe(ctl, sigframe_at);

	/*
	 * Lets open control socket.
	 */
	args = (prologue_init_args_t *)init_args_at;
	args->ctl_sock_addr_len = gen_std_saddr(&args->ctl_sock_addr, getpid());
	args->arg_s = arg_s;
	args->arg_p = ctl->remote_map + plugin_st.st_size;
	args->sigframe = (void *)ctl->remote_map + ((void *)sigframe_at - ctl->local_map);

	ctl->ctl_sock = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (ctl->ctl_sock < 0) {
		pr_perror("Can't create control socket");
		goto err;
	}

	if (bind(ctl->ctl_sock, (struct sockaddr *)&args->ctl_sock_addr, args->ctl_sock_addr_len) < 0) {
		pr_perror("Can't bind control socket");
		goto err;
	}

	if (listen(ctl->ctl_sock, 1) < 0) {
		pr_perror("Can't listen on control socket");
		goto err;
	}

	ret = setup_child_handler();
err:
	close(plugin_fd);
	if (ret) {
		parasite_ctl_fini(ctl);
		xfree(ctl);
		ctl = NULL;
	}
	return ctl;

err_restore:
	parasite_munmap_seized(ctl, ctl->remote_map, ctl->map_length);
	goto err;
}

parasite_ctl_t *parasite_start(pid_t pid, char *path, void *arg_p, unsigned int arg_s)
{
	user_regs_struct_t regs;
	parasite_ctl_t *ctl;
	int task_state;
	int ret = -1;

	task_state = seize_task(pid);
	if (task_state < 0) {
		pr_err("Can't seize task %d\n", pid);
		return ERR_PTR(-1);
	}

	if (task_in_compat_mode(pid)) {
		pr_err("Can't operate task %d in compatible mode\n", pid);
		goto err_unseize;
	}

	pr_debug("Initialize parasite control block\n");
	ctl = parasite_ctl_init(pid, path, arg_p, arg_s);
	if (!ctl)
		goto err_unseize;

	pr_debug("Execute parasite blob\n");
	regs = ctl->thread_ctx_orig.regs;
	if (parasite_run(pid, PTRACE_CONT, ctl->entry_ip, 0, &regs, &ctl->thread_ctx_orig)) {
		pr_err("Failed to continue running before sigreturn in %d\n", pid);
		goto recover;
	}

	pr_debug("Accepting connection\n");
	ctl->ctl_accepted = accept(ctl->ctl_sock, NULL, NULL);
	if (ctl->ctl_accepted < 0) {
		pr_perror("Failed to accept socket from %d\n", pid);
		goto recover;
	}

	ctl->task_state = task_state;
	return ctl;

recover:
	ret |= parasite_ctl_fini(ctl);
	xfree(ctl);

err_unseize:
	ret |= unseize_task(pid, task_state);
	return ERR_PTR(ret);
}

int parasite_end(parasite_ctl_t *ptr)
{
	parasite_ctl_t *ctl = ptr;
	int task_state;
	char data[32];
	int ret = -1;
	int status;
	pid_t pid;

	if (IS_ERR_OR_NULL(ctl))
		return (int)PTR_ERR(ctl);

	task_state = ctl->task_state;
	pid = ctl->pid;

	pr_debug("Waiting for control socket to shutdown\n");
	ret = recv(ctl->ctl_accepted, data, sizeof(data), MSG_WAITALL);
	if (ret != 0) {
		if (ret < 0)
			pr_perror("Error waiting for socket to shutdown");
		goto recover;
	}

	/* Stop getting chld from parasite -- we're about to step-by-step it */
	if (restore_child_handler())
		goto recover;

	/* Start to trace syscalls for each thread */
	pr_debug("Interrupt victim\n");
	if (ptrace(PTRACE_INTERRUPT, pid, NULL, NULL)) {
		pr_perror("Unable to interrupt the process");
		goto recover;
	}

	pr_debug("Waiting for %d to trap\n", pid);
	if (wait4(pid, &status, __WALL, NULL) != pid) {
		pr_perror("Waited pid mismatch (pid: %d)", pid);
		goto recover;
	}

	pr_debug("Victim %d exited trapping\n", pid);
	if (!WIFSTOPPED(status)) {
		pr_err("Task is still running (pid: %d)\n", pid);
		goto recover;
	}

	pr_debug("Shutdown control socket\n");
	ret = shutdown(ctl->ctl_accepted, SHUT_WR);
	if (ret) {
		pr_perror("Shutdown failed");
		goto recover;
	}

	pr_debug("Continue victim tracing syscalls\n");
	if (ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
		pr_perror("Can't continue victim with syscalls trasing %d", pid);
		goto recover;
	}

	pr_debug("Waiting for specific syscall in victim space\n");
	ret = -1;
	if (parasite_stop_on_syscall(1, __NR_rt_sigreturn)) {
		pr_err("Failed to stop on sigreturn in %d\n", pid);
		goto recover;
	}

	pr_debug("Execution is finished\n");
	ret = 0;

recover:
	ret |= parasite_ctl_fini(ctl);
	xfree(ctl);

	ret |= unseize_task(pid, task_state);
	return ret;
}
