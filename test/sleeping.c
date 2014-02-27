#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv)
{
	unsigned int counter = 5;
	unsigned long num = 0;
	double val = 0.0;

	while (counter--) {
		printf("%d: I'm alive (%li %g)\n", getpid(), num++, val);
		val += 0.1;
		sleep(1);
	}

	return 0;
}

