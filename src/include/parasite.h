#ifndef __COMPEL_PARASITE_H__
#define __COMPEL_PARASITE_H__

#include "asm-generic/int.h"
#include "asm/parasite.h"
#include "asm/ptrace.h"

#include "libcompel.h"

#define BUILTIN_SYSCALL_SIZE	8
#define COMPEL_ARGV_SIZE	1024

typedef struct parasite_ctl_s {
	pid_t		pid;
	int		task_state;

	unsigned long	syscall_ip;			/* where sys_mmap/munmap can run */
	unsigned long	entry_ip;			/* plugin entry point IP */
	unsigned long	sigframe_at;			/* plugin sigframe address */
	unsigned long	init_args_at;			/* plugin init argument address */

	int		ctl_sock;
	int		ctl_accepted;
	thread_ctx_t	thread_ctx_orig;
	u8		code_orig[BUILTIN_SYSCALL_SIZE];

	void		*remote_map;
	void		*local_map;
	unsigned long	map_length;
} parasite_ctl_t;

extern void *parasite_exec_start(pid_t pid, argv_desc_t *argv_desc);
extern int parasite_exec_end(void *ptr);
extern int parasite_exec(pid_t pid, argv_desc_t *argv_desc);
extern int parasite_execute_syscall_trap(parasite_ctl_t *ctl, user_regs_struct_t *regs);
extern int parasite_get_ctl_socket(void *ptr);
extern int parasite_get_rt_blob_desc(void *ptr, blob_rt_desc_t *d);

#endif /* __COMPEL_PARASITE_H__ */
