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

#include "libcompel.h"
#include "err.h"
#include "log.h"

#define COMPEL_DEFAULT_DIST	"./dist"
#define COMPEL_DEFAULT_LIB_DIR	"/usr/share/compel"
#define COMPEL_DEFAULT_PACK_OUT	"a.compel.o"

#define COMPEL_LDS		"pack.lds.S"
#define COMPEL_STD		"std.compel.o"

static int pack(char **obj, size_t nobj, const char *lib_path, char **libs, size_t nlibs, char *out)
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
	char *blob_args = NULL;
	size_t nlibs = 0;
	char *out = NULL;
	char *libs[512];
	pid_t pid = 0;

	const char short_opts[] = "t:l:v:o:L:a:";
	static struct option long_opts[] = {
		{ "tree",		required_argument,	0, 't' },
		{ "library",		required_argument,	0, 'l' },
		{ "out",		required_argument,	0, 'o' },
		{ "library-path",	required_argument,	0, 'L' },
		{ "version",		no_argument,		0, 'V' },
		{ "plugin-args",	required_argument,	0, 'a' },
		{ },
	};

	libcompel_log_set_fd(STDOUT_FILENO);

	while (1) {
		opt = getopt_long(argc, argv, short_opts, long_opts, &idx);
		if (opt == -1)
			break;

		switch (opt) {
		case 't':
			pid = (pid_t)atol(optarg);
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
		case 'V':
			pr_msg("version %s id %s\n", COMPEL_VERSION, COMPEL_GITID);
			return 0;
			break;
		case 'a':
			blob_args = optarg;
			break;
		case 'v':
			if (optarg)
				loglevel = atoi(optarg);
			else
				loglevel++;
			break;
		default:
			break;
		}
	}

	libcompel_loglevel_set(max(loglevel, (unsigned int)DEFAULT_LOGLEVEL));

	if (optind >= argc)
		goto usage;

	if (!strcmp(argv[optind], "run")) {
		argv_desc_t argv_desc = {
			.blob_args	= blob_args,
		};

		if (!pid || ++optind >= argc)
			goto usage;
		argv_desc.blob_path	= argv[optind];

		return libcompel_exec_elf(pid, &argv_desc);
	} else if (!strcmp(argv[optind], "pack")) {
		if (!nlibs || ++optind >= argc)
			goto usage;
		return pack(&argv[optind], argc - optind, lib_path, libs, nlibs, out);
	} else if (!strcmp(argv[optind], "cflags")) {
		pr_msg("-fpie -Wstrict-prototypes "
		       "-Wa,--noexecstack -fno-jump-tables "
		       "-nostdlib -fomit-frame-pointer");
		return 0;
	} else if (!strcmp(argv[optind], "ldflags")) {
		pr_msg("-r");
		return 0;
	}

usage:
	pr_msg("\nUsage:\n");
	pr_msg("  compel run <file> -t <pid>\n");
	pr_msg("  compel pack <file1> [<file2>] [-L<dir>] [-llib1] [-o out]\n");
	pr_msg("  compel cflags\n");
	pr_msg("  compel ldflags\n");
	return -1;
}
