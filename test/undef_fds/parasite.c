#include <compel/plugin-std.h>
#include <compel/plugin-fds.h>

int main(void *arg, unsigned int s)
{
	return fds_recv_one();
}
