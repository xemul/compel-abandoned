#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>

#include "asm/prologue.h"

#include "libcompel.h"
#include "parasite.h"
#include "compiler.h"
#include "xmalloc.h"
#include "types.h"
#include "elf.h"
#include "err.h"
#include "log.h"
#include "bug.h"

/*
 * Argvs are the following structure in memory
 * (summay max size is COMPEL_ARGV_SIZE)
 *
 * +------------------------------+
 * |      pointers to strings     |
 * +------------------------------+
 * |            strings           |
 * +------------------------------+
 */
int libcompel_fill_argv(parasite_ctl_t *ctl, load_info_t *info, argv_desc_t *d)
{
	prologue_init_args_t *args = (prologue_init_args_t *)ctl->init_args_at;
	char **ptrs = NULL, *str = NULL, *blob_name;
	size_t left = 0, zlen = 0;
	void *base;

	/*
	 * FIXME
	 *
	 * We need to fill arguments area but here is a small problem:
	 *  - the area is fixed in size
	 *  - the arguments look for us as a space separated string which
	 *    need to be splitted in getopt() fasion. So I think we need
	 *    to fill arguments in reserse order for easier poiners computation.
	 */

	if (!d->blob_args)
		return 0;

	args->argc = 1;
	args->argv = (void *)ctl->remote_map + info->len;
	base = (void *)args->argv + sizeof(char *);

	left = COMPEL_ARGV_SIZE - sizeof(char *);

	ptrs = (char **)(ctl->local_map + info->len);
	str = (void *)ptrs + sizeof(char *);

	/*
	 * Copy pluing name first.
	 */
	blob_name = basename(d->blob_path);
	zlen = strlen(blob_name) + 1;

	ptrs[0] = base;
	memcpy(str, blob_name, zlen);
	ptrs++, str += zlen, base += zlen;
	if (left < zlen)
		goto Enomem;
	left -= zlen;

	return 0;
Enomem:
	pr_err("No space left for plugins arguments\n");
	return -ENOMEM;
}
