#include <compel/plugin-std.h>
#include <compel/plugin-fds.h>

int main(void *arg, unsigned int s)
{
	int fd;

	fd = fds_recv_one();
	if (fd >= 0) {
		sys_write(fd, "P", 1);
		sys_close(fd);
	}

	return 0;
}
