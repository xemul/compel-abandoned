/*
 * Please add here type definitions if syscall prototypes need them.
 */

#ifndef __COMPEL_PLUGIN_STD_SYSCALL_TYPES_H__
#define __COMPEL_PLUGIN_STD_SYSCALL_TYPES_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sched.h>
#include <wait.h>

#include <linux/futex.h>

#include "uapi/asm-generic/int.h"
#include "uapi/asm/sigset.h"

struct file_handle;
struct itimerval;
struct rlimit64;

struct cap_header {
	u32 version;
	int pid;
};

struct cap_data {
	u32 eff;
	u32 prm;
	u32 inh;
};

struct krlimit {
	unsigned long rlim_cur;
	unsigned long rlim_max;
};

typedef struct {
	unsigned int	entry_number;
	unsigned int	base_addr;
	unsigned int	limit;
	unsigned int	seg_32bit:1;
	unsigned int	contents:2;
	unsigned int	read_exec_only:1;
	unsigned int	limit_in_pages:1;
	unsigned int	seg_not_present:1;
	unsigned int	useable:1;
	unsigned int	lm:1;
} user_desc_t;

#ifndef F_SETOWN_EX
#define F_SETOWN_EX	15
#define F_GETOWN_EX	16

struct f_owner_ex {
	int	type;
	pid_t	pid;
};
#endif

#ifndef F_GETOWNER_UIDS
#define F_GETOWNER_UIDS	17
#endif

#endif /* __COMPEL_PLUGIN_STD_SYSCALL_TYPES_H__ */
