#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "uapi/compel.h"
#include "shmem.h"
#include "parasite.h"

void *libcompel_shmem_create(compel_exec_handle_t h, unsigned long size)
{
	/* master -> parasite not implemented yet */
	return NULL;
}

void *libcompel_shmem_receive(compel_exec_handle_t h, unsigned long *size)
{
	parasite_ctl_t *ctl = (parasite_ctl_t *)h;
	struct shmem_plugin_msg spi;
	int fd;
	char ppath[128];
	void *mem;

	if (read(ctl_sock(ctl), &spi, sizeof(spi)) != sizeof(spi))
		return NULL;

	spi.len = round_up(spi.len, PAGE_SIZE);
	snprintf(ppath, sizeof(ppath), "/proc/%d/map_files/%lx-%lx",
		 ctl->pid, spi.start, spi.start + spi.len);
	fd = open(ppath, O_RDWR);
	if (fd < 0)
		return NULL;

	mem = mmap(NULL, spi.len, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (mem == MAP_FAILED)
		return NULL;

	*size = spi.len;
	return mem;
}
