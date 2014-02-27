#include <stdio.h>
#include <compel/compel.h>

static inline int check_pipe(int ctl, int rep, char c)
{
	int cr = 'X';

	if (ctl >= 0)
		write(ctl, &c, 1);
	read(rep, &cr, 1);
	if (cr != c)
		printf("Failed %c, has %c\n", c, cr);
	return cr != c;
}

int main(int argc, char **argv)
{
	int pid;
	int ctl[2], rep[2];
	char c;

	pipe(ctl);
	pipe(rep);

	printf("Spawning child\n");

	pid = fork();
	if (pid == 0) {
		/* I'm going to read from ctl */
		close(ctl[1]);
		/* And write into rep */
		close(rep[0]);

		/* Redirect FDs for parasite */
		dup2(ctl[0], 0);
		dup2(rep[1], 1);
		dup2(rep[1], 2);

		close(ctl[0]);
		close(rep[1]);

		while (1) {
			c = '\0';
			read(0, &c, 1);
			write(1, &c, 1);
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
	if (libcompel_exec(pid, argv[1], NULL, 0)) {
		printf("Failed to execute parasite\n");
		goto out;
	}

	if (check_pipe(-1, rep[0], 'P'))
		goto out;

	printf("Finishing child\n");
	if (check_pipe(ctl[1], rep[0], 'F'))
		goto out;

	printf("PASS\n");
out:
	kill(pid, 9);
	wait(NULL);
	return 0;
}
