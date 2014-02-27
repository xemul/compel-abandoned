#include <compel/plugin-std.h>

int main(void *arg, unsigned int s)
{
	int argc, i;
	char **argv;

	argc = std_argc(arg);
	argv = std_argv(arg, argc);

	for (i = 0; i < argc; i++)
		std_printf("%d = %s\n", i, argv[i]);

	return 0;
}
