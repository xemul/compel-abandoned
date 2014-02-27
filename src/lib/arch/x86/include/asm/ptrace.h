#ifndef __ASM_LIBCOMPEL_PTRACE_H__
#define __ASM_LIBCOMPEL_PTRACE_H__

#include <stdbool.h>

#include "asm/types.h"

#define ARCH_SI_TRAP SI_KERNEL

extern bool task_in_compat_mode(pid_t pid);
extern int ptrace_get_fpu(pid_t pid, fpu_state_t *fpu);
extern int ptrace_set_fpu(pid_t pid, fpu_state_t *fpu);

#endif /* __ASM_LIBCOMPEL_PTRACE_H__ */
