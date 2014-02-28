#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include <getopt.h>

#include "compiler.h"
#include "version.h"
#include "xmalloc.h"

#include "uapi/compel.h"
#include "err.h"
#include "log.h"

#define COMPEL_DEFAULT_DIST	"./dist"
#define COMPEL_DEFAULT_LIB_DIR	"/usr/share/compel"
#define COMPEL_DEFAULT_PACK_OUT	"a.compel.o"

#define COMPEL_LDS		"pack.lds.S"
#define COMPEL_STD		"std.compel.o"

static int pack_compel_obj(char **obj, size_t nobj,
		const char *lib_path, char **libs, size_t nlibs, char *out)
{
	char command[1024] = { 0 };
	char data[1024] = { 0 };
	int ret = -1, pos, i;
	FILE *f;

	/*
	 * Figure out where the scripts we need are sitting.
	 */
	if (!lib_path) {
		if (access(COMPEL_DEFAULT_DIST "/" COMPEL_LDS, R_OK) &&
		    access(COMPEL_DEFAULT_DIST "/" COMPEL_STD, R_OK)) {
			lib_path = COMPEL_DEFAULT_DIST;
		} else {
			if (access(COMPEL_DEFAULT_LIB_DIR "/" COMPEL_LDS, R_OK) &&
			    access(COMPEL_DEFAULT_LIB_DIR "/" COMPEL_STD, R_OK)) {
				lib_path = COMPEL_DEFAULT_DIST;
			} else
				goto err_no_res;
		}
	}

	pos = snprintf(command, sizeof(command),
		       "ld -r -T %s/" COMPEL_LDS " -o %s %s/" COMPEL_STD,
		       lib_path, out ? : COMPEL_DEFAULT_PACK_OUT, lib_path);

	for (i = 0; pos < sizeof(command) - 1 && i < nobj; i++)
		pos += snprintf(&command[pos], sizeof(command) - pos - 1,
				" %s", obj[i]);

	for (i = 0; pos < sizeof(command) - 1 && i < nlibs; i++)
		pos += snprintf(&command[pos], sizeof(command) - pos - 1,
				" %s/%s.compel.o", lib_path, libs[i]);

	pr_debug("pack as: `%s'\n", command);

	f = popen(command, "r");
	if (f) {
		fgets(data, sizeof(data), f);
		ret = data[0] == '\0' ? 0 : 1;
		pclose(f);
		if (!ret)
			ret = libcompel_verify_packed(out ? : COMPEL_DEFAULT_PACK_OUT);
	} else
		pr_perror("Can't run pack");
	if (ret)
		pr_err("Pack failed\n");
	return ret;

err_no_res:
	pr_err("Can't find resource files needed for `pack' action\n");
	return -1;
}

int main(int argc, char *argv[])
{
	unsigned int loglevel = DEFAULT_LOGLEVEL;
	int opt, idx;

	char *lib_path = NULL;
	size_t nlibs = 0;
	char *out = NULL;
	char *libs[512];
	pid_t pid = 0;
	char *action;
	char *blob = NULL;

	const char short_opts[] = "f:p:l:v::o:L:Vh";
	static struct option long_opts[] = {
		/*
		 * Options for run
		 */
		{ "pid",		required_argument,	0, 'p' },
		{ "cfile",		required_argument,	0, 'f' },

		/*
		 * Options for pack
		 */
		{ "library",		required_argument,	0, 'l' },
		{ "out",		required_argument,	0, 'o' },
		{ "library-path",	required_argument,	0, 'L' },

		/*
		 * Other options
		 */
		{ "version",		no_argument,		0, 'V' },
		{ "help",		no_argument,		0, 'h' },
		{ },
	};

	while (1) {
		opt = getopt_long(argc, argv, short_opts, long_opts, &idx);
		if (opt == -1)
			break;

		switch (opt) {
		case 'p':
			pid = (pid_t)atol(optarg);
			break;
		case 'f':
			blob = optarg;
			break;
		case 'l':
			if (nlibs >= ARRAY_SIZE(libs)) {
				pr_err("Too many libraries specified, max %d\n",
				       (int)ARRAY_SIZE(libs));
				exit(1);
			}
			libs[nlibs++] = optarg;
			break;
		case 'o':
			out = optarg;
			break;
		case 'L':
			/*
			 * FIXME Maybe we need to allow multiple -L ?
			 */
			lib_path = optarg;
			break;
		case 'v':
			if (optarg)
				loglevel = atoi(optarg);
			else
				loglevel++;
			break;
		default:
			break;

		case 'V':
			printf("version %s id %s\n", COMPEL_VERSION, COMPEL_GITID);
			return 0;
		case 'h':
			goto usage;
		}
	}

	libcompel_log_init(STDOUT_FILENO,
			max(loglevel, (unsigned int)DEFAULT_LOGLEVEL));

	if (optind >= argc)
		goto usage;

	action = argv[optind++];

	if (!strcmp(action, "run")) {
		void *arg_p;
		unsigned int arg_s;

		if (!pid || !blob)
			goto usage;

		if (libcompel_pack_argv(blob, argc - optind, argv + optind, &arg_p, &arg_s))
			return 1;

		return libcompel_exec(pid, blob, arg_p, arg_s);
	}

	if (!strcmp(action, "pack")) {
		if (optind >= argc)
			goto usage;

		return pack_compel_obj(argv + optind, argc - optind,
				lib_path, libs, nlibs, out);
	}

	if (!strcmp(action, "cflags")) {
		printf("-fpie -Wstrict-prototypes "
		       "-Wa,--noexecstack -fno-jump-tables "
		       "-nostdlib -fomit-frame-pointer");
		return 0;
	}

	if (!strcmp(action, "ldflags")) {
		printf("-r");
		return 0;
	}

usage:
	printf("Usage:\n");
	printf("  compel run -f <file> -t <pid>\n");
	printf("  compel pack <files...> [-L<dir>] [-llibs] [-o out]\n");
	printf("  compel cflags\n");
	printf("  compel ldflags\n");
	return -1;
}
