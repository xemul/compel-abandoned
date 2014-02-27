#ifndef __UAPI_ASM_LIBCOMPEL_SIGSET_H__
#define __UAPI_ASM_LIBCOMPEL_SIGSET_H__

#include <signal.h>

typedef void rt_signalfn_t(int, siginfo_t *, void *);
typedef rt_signalfn_t *rt_sighandler_t;

typedef void rt_restorefn_t(void);
typedef rt_restorefn_t *rt_sigrestore_t;

#define _KNSIG		64
#define _NSIG_BPW	64

#define _KNSIG_WORDS     (_KNSIG / _NSIG_BPW)

typedef struct {
	unsigned long sig[_KNSIG_WORDS];
} k_rtsigset_t;

static inline void ksigfillset(k_rtsigset_t *set)
{
	int i;
	for (i = 0; i < _KNSIG_WORDS; i++)
		set->sig[i] = -1ul;
}

typedef struct {
	rt_sighandler_t	rt_sa_handler;
	unsigned long	rt_sa_flags;
	rt_sigrestore_t	rt_sa_restorer;
	k_rtsigset_t	rt_sa_mask;
} rt_sigaction_t;

#endif /* __UAPI_ASM_LIBCOMPEL_SIGSET_H__ */
