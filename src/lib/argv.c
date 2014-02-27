#include <stdlib.h>
#include <string.h>
#include "uapi/compel.h"
#include "parasite.h"
#include "elf.h"

int libcompel_pack_argv(char *blob, int argc, char **argv, void **arg_p, unsigned int *arg_s)
{
	int i, *argc_packed;
	char *args_mem;
	unsigned total_len;
	unsigned long *argv_packed;

	total_len = sizeof(int);
	total_len += sizeof(unsigned long) + strlen(blob) + 1;
	for (i = 0; i < argc; i++)
		total_len += sizeof(unsigned long) + strlen(argv[i]) + 1;

	argc_packed = malloc(total_len);
	if (!argc_packed)
		return -1;

	*argc_packed = argc + 1;
	argv_packed = (unsigned long *)(argc_packed + 1);
	args_mem = (char *)(argv_packed + argc + 1);
	argv_packed[0] = (void *)args_mem - (void *)argc_packed;
	args_mem = stpcpy(args_mem, blob) + 1;
	for (i = 0; i < argc; i++) {
		argv_packed[i + 1] = (void *)args_mem - (void *)argc_packed;
		args_mem = stpcpy(args_mem, argv[i]) + 1;
	}

	*arg_p = argc_packed;
	*arg_s = total_len;

	return 0;
}
