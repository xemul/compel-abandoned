define STRLCPY_TEST

#include <string.h>

int main(void)
{
	char src[32] = "strlcpy";
	char dst[32];

	return strlcpy(dst, src, sizeof(dst));
}
endef

define STRLCAT_TEST

#include <string.h>

int main(void)
{
	char src[32] = "strlcat";
	char dst[32] = '\0';

	return strlcat(dst, src, sizeof(dst));
}
endef
