#include <compel/plugin-std.h>

int main(void *arg, unsigned int s)
{
	char c = 'X';
	sys_read(0, &c, 1);
	sys_write(1, &c, 1);
	return 0;
}
