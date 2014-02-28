#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "uapi/compel.h"
#include "compiler.h"
#include "parasite.h"
#include "version.h"
#include "xmalloc.h"
#include "ptrace.h"
#include "types.h"
#include "log.h"
#include "err.h"
#include "elf.h"

void libcompel_version(unsigned int *major, unsigned int *minor, unsigned int *sublevel)
{
	*major = COMPEL_VERSION_MAJOR;
	*minor = COMPEL_VERSION_MINOR;
	*sublevel = COMPEL_VERSION_SUBLEVEL;
}

compel_exec_handle_t libcompel_exec_start(pid_t pid, char *path, void *arg_p, unsigned int arg_s, int *err)
{
	parasite_ctl_t *p;

	p = parasite_start(pid, path, arg_p, arg_s);
	if (IS_ERR(p)) {
		if (err)
			*err = PTR_ERR(p);
		p = NULL;
	}

	return (compel_exec_handle_t)p;
}

int libcompel_exec_end(compel_exec_handle_t p)
{
	return parasite_end((parasite_ctl_t *)p);
}

int libcompel_exec(pid_t pid, char *path, void *arg_p, unsigned int arg_s)
{
	void *p;

	p = parasite_start(pid, path, arg_p, arg_s);
	if (IS_ERR(p))
		return PTR_ERR(p);

	return parasite_end(p);
}

int libcompel_verify_packed(char *path)
{
	return verify_elf_packed(path);
}
