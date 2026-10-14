#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define M 1048576

static void	print(char *s)
{
	write(1, s, strlen(s));
}

int	main(void)
{
	char	*addr1;
	char	*addr2;
	char	*addr3;

	addr1 = (char *)malloc(16 * M);
	strcpy(addr1, "Bonjour\n");
	print(addr1);
	addr2 = (char *)malloc(16 * M);
	addr3 = (char *)realloc(addr1, 128 * M);
	addr3[127 * M] = 42;
	print(addr3);
	(void)addr2;
	return (0);
}
