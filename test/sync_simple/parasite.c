#include <compel/plugin-std.h>

int main(void *arg, unsigned int s)
{
	char c = 'P';
	sys_write(1, &c, 1);
	return 0;
}
