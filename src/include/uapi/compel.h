/*
 * compel.h -- header describing API to compel library.
 */

#ifndef __UAPI_LIBCOMPEL_H__
#define __UAPI_LIBCOMPEL_H__

#include <sys/types.h>

struct compel_exec_handle;
typedef struct compel_exec_handle *compel_exec_handle_t;

/*
 * Service calls
 */

extern void libcompel_version(unsigned int *major, unsigned int *minor, unsigned int *sublevel);
extern void libcompel_log_init(int fd, unsigned int level);
extern int libcompel_pack_argv(char *blob, int argc, char **argv, void **arg_p, unsigned int *arg_s);

/*
 * Main parasite run routines
 */

extern int libcompel_exec(pid_t pid, char *path, void *arg_p, unsigned int arg_s);
extern compel_exec_handle_t libcompel_exec_start(pid_t pid, char *path, void *arg_p, unsigned int arg_s, int *err);
extern int libcompel_exec_end(compel_exec_handle_t h);

/*
 * Plugins verification
 */
extern int libcompel_verify_packed(char *path);

/*
 * Communication with parasite
 */

extern int libcompel_send_fds(compel_exec_handle_t h, int *fds, int nr);
extern int libcompel_send_fd(compel_exec_handle_t h, int fd);
extern int libcompel_recv_fds(compel_exec_handle_t h, int *fds, int nr);
extern int libcompel_recv_fd(compel_exec_handle_t h);

extern void *libcompel_shmem_create(compel_exec_handle_t h, unsigned long size);
extern void *libcompel_shmem_receive(compel_exec_handle_t h, unsigned long *size);

#endif /* __UAPI_LIBCOMPEL_H__ */
