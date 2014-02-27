#ifndef __UAPI_LIBCOMPEL_H__
#define __UAPI_LIBCOMPEL_H__

#include <sys/types.h>

typedef struct blob_rt_desc_s {
	pid_t		pid;

	unsigned long	init_args_at;
	size_t		init_args_size;

	void		*remote_map;
	void		*local_map;
} blob_rt_desc_t;

typedef struct argv_desc_s {
	char		*blob_path;
	char		*blob_args;
} argv_desc_t;

#define LIBCOMPEL_VERSION_SYM		"libcompel_version"
typedef void (*libcompel_version_t)(unsigned int *major, unsigned int *minor, unsigned int *sublevel);
extern void libcompel_version(unsigned int *major, unsigned int *minor, unsigned int *sublevel);

#define LIBCOMPEL_EXEC_ELF_SYM		"libcompel_exec_elf"
typedef int (*libcompel_exec_elf_t)(pid_t pid, argv_desc_t *argv_desc);
extern int libcompel_exec_elf(pid_t pid, argv_desc_t *argv_desc);

#define LIBCOMPEL_EXEC_START_SYM	"libcompel_exec_start"
typedef void *(*libcompel_exec_start_t)(pid_t pid, argv_desc_t *argv_desc, int *err);
extern void *libcompel_exec_start(pid_t pid, argv_desc_t *argv_desc, int *err);

#define LIBCOMPEL_EXEC_END_SYM		"libcompel_exec_end"
typedef int (*libcompel_exec_end_t)(void *ptr);
extern int libcompel_exec_end(void *ptr);

#define LIBCOMPEL_LOG_SET_FD_SYM	"libcompel_log_set_fd"
typedef int (*libcompel_log_set_fd_t)(int fd);
extern void libcompel_log_set_fd(int fd);

#define LIBCOMPEL_LOGLEVEL_SET_SYM	"libcompel_loglevel_set"
typedef int (*libcompel_loglevel_set_t)(unsigned int loglevel);
extern void libcompel_loglevel_set(unsigned int loglevel);

#define LIBCOMPEL_GET_CTL_SOCKET_SYM	"libcompel_get_ctl_socket"
typedef int (*libcompel_get_ctl_socket_t)(void *ptr);
extern int libcompel_get_ctl_socket(void *ptr);

#define LIBCOMPEL_GET_RT_BLOB_DESC_SYM	"libcompel_get_rt_blob_desc"
typedef int (*libcompel_get_rt_blob_desc_t)(void *ptr, blob_rt_desc_t *d);
extern int libcompel_get_rt_blob_desc(void *ptr, blob_rt_desc_t *d);

#endif /* __UAPI_LIBCOMPEL_H__ */
