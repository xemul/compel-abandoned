#include <compel/plugin-std.h>
#include <compel/plugin-fds.h>
#include <compel/plugin-shmem.h>

int main(void *arg, unsigned int s)
{
	int fd;
	char *mem;

	mem = shmem_create(16);
	if (mem == NULL)
		return 0;

	fd = fds_recv_one();
	if (fd < 0)
		return 0;

	sys_read(fd, mem, 16);
	sys_munmap(mem, 16);
	sys_close(fd);
	return 0;
}
