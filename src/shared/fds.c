#include <stdbool.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>

/*
 * Because of kernel doing kmalloc for user data passed
 * in SCM messages, and there is kernel's SCM_MAX_FD as a limit
 * for descriptors passed at once we're trying to reduce
 * the pressue on kernel memory manager and use predefined
 * known to work well size of the message buffer.
 */
#define CR_SCM_MSG_SIZE		(1024)
#define CR_SCM_MAX_FD		(252)

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

struct fd_opts {
	char		flags;
	struct {
		u32	uid;
		u32	euid;
		u32	signum;
		u32	pid_type;
		u32	pid;
	} fown;
};

struct scm_fdset {
	struct msghdr	hdr;
	struct iovec	iov;
	char		msg_buf[CR_SCM_MSG_SIZE];
	struct fd_opts	opts[CR_SCM_MAX_FD];
};

static void scm_fdset_init_chunk(struct scm_fdset *fdset, int nr_fds)
{
	struct cmsghdr *cmsg;

	fdset->hdr.msg_controllen = CMSG_LEN(sizeof(int) * nr_fds);

	cmsg		= CMSG_FIRSTHDR(&fdset->hdr);
	cmsg->cmsg_len	= fdset->hdr.msg_controllen;
}

static int *scm_fdset_init(struct scm_fdset *fdset, bool with_flags)
{
	struct cmsghdr *cmsg;

	BUILD_BUG_ON(sizeof(fdset->msg_buf) < (CMSG_SPACE(sizeof(int) * CR_SCM_MAX_FD)));

	fdset->iov.iov_base		= fdset->opts;
	fdset->iov.iov_len		= with_flags ? sizeof(fdset->opts) : 1;

	fdset->hdr.msg_iov		= &fdset->iov;
	fdset->hdr.msg_iovlen		= 1;
	fdset->hdr.msg_name		= NULL;
	fdset->hdr.msg_namelen		= 0;

	fdset->hdr.msg_control		= &fdset->msg_buf;
	fdset->hdr.msg_controllen	= CMSG_LEN(sizeof(int) * CR_SCM_MAX_FD);

	cmsg				= CMSG_FIRSTHDR(&fdset->hdr);
	cmsg->cmsg_len			= fdset->hdr.msg_controllen;
	cmsg->cmsg_level		= SOL_SOCKET;
	cmsg->cmsg_type			= SCM_RIGHTS;

	return (int *)CMSG_DATA(cmsg);
}

int fds_send_via(int sock, int *fds, int nr_fds, bool with_flags)
{
	struct scm_fdset fdset;
	int i, min_fd, ret;
	int *cmsg_data;

	cmsg_data = scm_fdset_init(&fdset, with_flags);

	for (i = 0; i < nr_fds; i += min_fd) {
		min_fd = min(CR_SCM_MAX_FD, nr_fds - i);
		scm_fdset_init_chunk(&fdset, min_fd);
		__std(memcpy(cmsg_data, &fds[i], sizeof(int) * min_fd));

		if (with_flags) {
			int j;

			for (j = 0; j < min_fd; j++) {
				int flags, fd = fds[i + j];
				struct fd_opts *p = fdset.opts + j;
				struct f_owner_ex owner_ex;
				u32 v[2];

				flags = __sys(fcntl(fd, F_GETFD, 0));
				if (flags < 0)
					return -1;

				p->flags = (char)flags;

				if (__sys(fcntl(fd, F_GETOWN_EX, (long)&owner_ex)))
					return -1;

				/*
				 * Simple case -- nothing is changed.
				 */
				if (owner_ex.pid == 0) {
					p->fown.pid = 0;
					continue;
				}

				if (__sys(fcntl(fd, F_GETOWNER_UIDS, (long)&v)))
					return -1;

				p->fown.uid	 = v[0];
				p->fown.euid	 = v[1];
				p->fown.pid_type = owner_ex.type;
				p->fown.pid	 = owner_ex.pid;
			}
		}

		ret = __sys(sendmsg(sock, &fdset.hdr, 0));
		if (ret <= 0)
			return ret ? : -1;
	}

	return 0;
}

int fds_recv_via(int sock, int *fds, int nr_fds, struct fd_opts *opts)
{
	struct scm_fdset fdset;
	struct cmsghdr *cmsg;
	int *cmsg_data;
	int ret;
	int i, min_fd;

	cmsg_data = scm_fdset_init(&fdset, opts != NULL);
	for (i = 0; i < nr_fds; i += min_fd) {
		min_fd = min(CR_SCM_MAX_FD, nr_fds - i);
		scm_fdset_init_chunk(&fdset, min_fd);

		ret = __sys(recvmsg(sock, &fdset.hdr, 0));
		if (ret <= 0)
			return ret ? : -1;

		cmsg = CMSG_FIRSTHDR(&fdset.hdr);
		if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS)
			return -EINVAL;
		if (fdset.hdr.msg_flags & MSG_CTRUNC)
			return -ENFILE;

		min_fd = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
		/*
		 * In case if kernel screwed the recepient, most probably
		 * the caller stack frame will be overwriten, just scream
		 * and exit.
		 */
		if (unlikely(min_fd > CR_SCM_MAX_FD))
			*(volatile unsigned long *)NULL = 0xdead0000 + __LINE__;
		if (unlikely(min_fd <= 0))
			return -1;
		__std(memcpy(&fds[i], cmsg_data, sizeof(int) * min_fd));
		if (opts)
			__std(memcpy(opts + i, fdset.opts, sizeof(struct fd_opts) * min_fd));
	}

	return 0;
}
