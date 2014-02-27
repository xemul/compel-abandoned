#ifndef __ASM_LIBCOMPEL_TYPES_H__
#define __ASM_LIBCOMPEL_TYPES_H__

#include "asm-generic/int.h"

#include "asm/regs.h"
#include "asm/sigset.h"
#include "asm/page.h"
#include "asm/fpu.h"

typedef struct {
	k_rtsigset_t		sigmask;
	user_regs_struct_t	regs;
	fpu_state_t		fpu;
} thread_ctx_t;

#define TASK_SIZE		((1UL << 47) - PAGE_SIZE)

#endif /* __ASM_LIBCOMPEL_TYPES_H__ */
