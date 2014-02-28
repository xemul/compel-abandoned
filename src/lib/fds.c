#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "parasite.h"
#include "asm-generic/int.h"
#include "uapi/compel.h"
#include "compiler.h"

#define __sys(foo)	foo
#define __std(foo)	foo

#include "../shared/fds.c"

int libcompel_send_fds(compel_exec_handle_t h, int *fds, int nr)
{
	return fds_send_via(ctl_sock((parasite_ctl_t *)h), fds, nr);
}

int libcompel_send_fd(compel_exec_handle_t h, int fd)
{
	return libcompel_send_fds(h, &fd, 1);
}

int libcompel_recv_fds(compel_exec_handle_t h, int *fds, int nr)
{
	return fds_recv_via(ctl_sock((parasite_ctl_t *)h), fds, nr);
}

int libcompel_recv_fd(compel_exec_handle_t h)
{
	int fd, ret;

	ret = libcompel_recv_fds(h, &fd, 1);
	if (ret)
		fd = -1;

	return fd;
}
