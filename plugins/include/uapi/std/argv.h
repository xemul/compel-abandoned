#ifndef __COMPEL_PLUGIN_STD_ARGV_H__
#define __COMPEL_PLUGIN_STD_ARGV_H__

static inline int std_argc(void *arg_p)
{
	return *(int *)arg_p;
}

static inline char **std_argv(void *arg_p, int argc)
{
	int i;
	unsigned long *argv = (unsigned long *)(arg_p + sizeof(int));

	for (i = 0; i < argc; i++)
		argv[i] = (unsigned long)(arg_p + argv[i]);

	return (char **)argv;
}

#endif /* __COMPEL_PLUGIN_STD_ARGV_H__ */
