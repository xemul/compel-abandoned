#include <stdio.h>
#include <string.h>
#include <compel/compel.h>

static inline int check_pipe(int ctl, int rep, char c)
{
	int cr = 'X';

	write(ctl, &c, 1);
	read(rep, &cr, 1);
	if (cr != c)
		printf("Failed %c, has %c\n", c, cr);
	return cr != c;
}

static char *message = "0123456789ABCDEF";

int main(int argc, char **argv)
{
	int pid;
	int ctl[2], rep[2], p_ctl[2];
	char c;
	char *p_shmem;
	unsigned long p_sh_size;
	compel_exec_handle_t p;

	pipe(ctl);
	pipe(rep);
	pipe(p_ctl);

	printf("Spawning child\n");

	pid = fork();
	if (pid == 0) {
		/* I'm going to read from ctl */
		close(ctl[1]);
		/* And write into rep */
		close(rep[0]);

		while (1) {
			c = '\0';
			read(ctl[0], &c, 1);
			write(rep[1], &c, 1);
			if (c == 'F')
				break;
		}

		return 0;
	}

	/* I'm going to write into ctl */
	close(ctl[0]);
	/* And read from rep */
	close(rep[1]);

	printf("Starting child\n");
	if (check_pipe(ctl[1], rep[0], 'S'))
		goto out;

	printf("Running parasite\n");
	p = libcompel_exec_start(pid, argv[1], NULL, 0, NULL);
	if (!p) {
		printf("Failed to start parasite\n");
		goto out;
	}

	printf("Sending pipe to parasite\n");
	if (libcompel_send_fd(p, p_ctl[0])) {
		printf("Failed to send fd\n");
		goto out;
	}

	close(p_ctl[0]);

	printf("Getting shmem from parasite\n");

	p_shmem = libcompel_shmem_receive(p, &p_sh_size);
	if (p_shmem == NULL) {
		printf("Failed to get shmem\n");
		goto out;
	}

	printf("\tShmem contains [%c%c%c...]\n",
			p_shmem[0], p_shmem[1], p_shmem[2]);

	write(p_ctl[1], message, 16);

	if (libcompel_exec_end(p)) {
		printf("Failed to unload parasite\n");
		goto out;
	}

	printf("\tShmem filled with [%8s...]\n", p_shmem);

	if (strncmp(p_shmem, message, 16)) {
		printf("Message differs\n");
		goto out;
	}

	munmap(p_shmem, p_sh_size);

	printf("Finishing child\n");
	if (check_pipe(ctl[1], rep[0], 'F'))
		goto out;

	printf("PASS\n");
out:
	kill(pid, 9);
	wait(NULL);
	return 0;
}
