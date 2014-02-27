#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <fcntl.h>

#include "compiler.h"
#include "types.h"

#include "log.h"

static unsigned int current_loglevel = DEFAULT_LOGLEVEL;
static char logbuf[PAGE_SIZE];
static int logfd = -1;

void log_set_fd(int fd)
{
	if (logfd != -1)
		close(logfd);
	logfd = fd;
}

void loglevel_set(unsigned int loglevel)
{
	current_loglevel = loglevel;
}

unsigned int loglevel_get(void)
{
	return current_loglevel;
}

bool pr_quelled(unsigned int loglevel)
{
	return loglevel != LOG_MSG && loglevel > loglevel_get();
}

static void __print_on_level(unsigned int loglevel, const char *format, va_list params)
{
	size_t size;

	if (logfd < 0)
		return;

	size = vsnprintf(logbuf, PAGE_SIZE, format, params);
	write(logfd, logbuf, size);
}

void print_on_level(unsigned int loglevel, const char *format, ...)
{
	va_list params;

	if (pr_quelled(loglevel))
		return;

	va_start(params, format);
	__print_on_level(loglevel, format, params);
	va_end(params);
}
