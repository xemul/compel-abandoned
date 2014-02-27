#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>

#include "compel/libcompel-plugin.h"

#include "compel/std/syscall.h"
#include "compel/std/string.h"
#include "compel/std/std.h"

static const char msg[] = "Hello from libcompel";

static const char *func(void)
{

	/* A function to make "main" offsetted */
	return msg;
}

int main(int argc, char *argv[])
{
	const char *s;
	int i;

	std_printf("simple plugin: main\n");

	std_printf("simple plugin: message \"%s\"\n", func());
	std_printf("simple plugin: string \"%s\" int %d long %ld hex %lx\n",
		   func(), 123, 1099511627775ul, 0x123456789abcdef0ul);

	s = "0x1234";
	std_printf("simple plugin: `%s' -> %ld\n", s, std_strtoul(s, 0, 0));

	s = "0b1111101110";
	std_printf("simple plugin: `%s' -> %ld\n", s, std_strtoul(s, 0, 0));

	s = "011111";
	std_printf("simple plugin: `%s' -> %ld\n", s, std_strtoul(s, 0, 16));

	s = "-011111";
	std_printf("simple plugin: `%s' -> %ld\n", s, std_strtoul(s, 0, 2));

	for (i = 0; i < argc; i++)
		std_printf("simple plugin: argc = %d argv = %s\n", i, argv[i]);
	return 0;
}
