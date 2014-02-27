#ifndef __COMPEL_PTRACE_H__
#define __COMPEL_PTRACE_H__

#include "asm-generic/int.h"
#include "asm/ptrace.h"

#define TASK_ALIVE		0x1
#define TASK_DEAD		0x2
#define TASK_STOPPED		0x3
#define TASK_HELPER		0x4

/* some constants for ptrace */
#ifndef PTRACE_SEIZE
# define PTRACE_SEIZE		0x4206
#endif

#ifndef PTRACE_INTERRUPT
# define PTRACE_INTERRUPT	0x4207
#endif

#ifndef PTRACE_LISTEN
# define PTRACE_LISTEN		0x4208
#endif

#ifndef PTRACE_PEEKSIGINFO
#define PTRACE_PEEKSIGINFO      0x4209
struct ptrace_peeksiginfo_args {
        u64 off;	/* from which siginfo to start */
        u32 flags;
        u32 nr;		/* how may siginfos to take */
};

/* Read signals from a shared (process wide) queue */
#define PTRACE_PEEKSIGINFO_SHARED       (1 << 0)
#endif

#ifndef PTRACE_GETREGSET
# define PTRACE_GETREGSET	0x4204
# define PTRACE_SETREGSET	0x4205
#endif

#define PTRACE_GETSIGMASK	0x420a
#define PTRACE_SETSIGMASK	0x420b

#define PTRACE_SEIZE_DEVEL	0x80000000

#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6
#define PTRACE_EVENT_STOP	128

#define PTRACE_O_TRACESYSGOOD	0x00000001
#define PTRACE_O_TRACEFORK	0x00000002
#define PTRACE_O_TRACEVFORK	0x00000004
#define PTRACE_O_TRACECLONE	0x00000008
#define PTRACE_O_TRACEEXEC	0x00000010
#define PTRACE_O_TRACEVFORKDONE	0x00000020
#define PTRACE_O_TRACEEXIT	0x00000040

#define SI_EVENT(_si_code)	(((_si_code) & 0xFFFF) >> 8)

extern int unseize_task(pid_t pid, int state);
extern int seize_task(pid_t pid);

extern int ptrace_peek_area(pid_t pid, void *dst, void *addr, unsigned long bytes);
extern int ptrace_poke_area(pid_t pid, void *src, void *addr, unsigned long bytes);
extern int ptrace_swap_area(pid_t pid, void *dst, void *src, unsigned long bytes);

extern int ptrace_get_regs(int pid, user_regs_struct_t *regs);
extern int ptrace_set_regs(int pid, user_regs_struct_t *regs);
extern int ptrace_get_thread_ctx(pid_t pid, thread_ctx_t *ctx);
extern int ptrace_set_thread_ctx(pid_t pid, thread_ctx_t *ctx);

#endif /* __COMPEL_PTRACE_H__ */
