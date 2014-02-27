#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "uapi/libcompel.h"

#include "libcompel.h"
#include "compiler.h"
#include "parasite.h"
#include "version.h"
#include "xmalloc.h"
#include "ptrace.h"
#include "types.h"
#include "log.h"
#include "err.h"
#include "elf.h"

void libcompel_log_set_fd(int fd)
{
	log_set_fd(fd);
}

void libcompel_loglevel_set(unsigned int loglevel)
{
	loglevel_set(loglevel);
}

int libcompel_get_rt_blob_desc(void *ptr, blob_rt_desc_t *d)
{
	return parasite_get_rt_blob_desc(ptr, d);
}

int libcompel_get_ctl_socket(void *ptr)
{
	return parasite_get_ctl_socket(ptr);
}

void libcompel_version(unsigned int *major, unsigned int *minor, unsigned int *sublevel)
{
	*major = COMPEL_VERSION_MAJOR;
	*minor = COMPEL_VERSION_MINOR;
	*sublevel = COMPEL_VERSION_SUBLEVEL;
}

void *libcompel_exec_start(pid_t pid, argv_desc_t *argv_desc, int *err)
{
	void *p = parasite_exec_start(pid, argv_desc);

	if (IS_ERR_OR_NULL(p)) {
		if (err)
			*err = PTR_ERR(p);
		p = NULL;
	}

	return p;
}

int libcompel_exec_end(void *ptr)
{
	return parasite_exec_end(ptr);
}

int libcompel_exec_elf(pid_t pid, argv_desc_t *argv_desc)
{
#if 0
	if (!ret) {
		/*
		 * Check if we support this version at all.
		 */
		unsigned int version = *(unsigned int *)&((char *)d.blob)[d.version_at];
		if (version > COMPEL_VERSION_CODE) {
			pr_err("Unsupported version %x of elf blob\n", version);
			ret = -1;
		} else
			pr_debug("Blob version %x libversion %x\n", version, COMPEL_VERSION_CODE);
	}
#endif
	return parasite_exec(pid, argv_desc);
}
