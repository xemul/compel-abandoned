#ifndef __ASM_LIBCOMPEL_PARASITE_H__
#define __ASM_LIBCOMPEL_PARASITE_H__

#include "asm-generic/int.h"
#include "asm/regs.h"
#include "asm/types.h"

struct parasite_ctl_s;

#define ASM_RAW_SYSCALL												\
{															\
	0x0f, 0x05,				/* syscall    */							\
	0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc	/* int 3, ... */							\
}

#define ASM_RAW_SYSCALL_SIZE 8

extern void parasite_setup_regs(unsigned long new_ip, void *stack, user_regs_struct_t *regs);
extern void parasite_fixup_thread_ctx(thread_ctx_t *ctx);
extern void parasite_prepare_sigframe(void *ctl);
extern int parasite_syscall_seized(struct parasite_ctl_s *ctl,
				   int nr, unsigned long *ret,
				   unsigned long arg1,
				   unsigned long arg2,
				   unsigned long arg3,
				   unsigned long arg4,
				   unsigned long arg5,
				   unsigned long arg6);

#define parasite_is_service_vma(path)		\
	(!strcmp(path, "[vdso]") || !strcmp(path, "[vsyscall]"))

#endif /* __ASM_LIBCOMPEL_PARASITE_H__ */
