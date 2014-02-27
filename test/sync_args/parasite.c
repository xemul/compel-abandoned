#include <compel/plugin-std.h>

int main(void *arg, unsigned int s)
{
	char c = *(char *)arg;
	sys_write(1, &c, 1);
	return 0;
}
