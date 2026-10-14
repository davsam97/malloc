#include "malloc.h"
#include <string.h>

int	main(void)
{
	char	*a;
	char	*b;
	char	*c;

	a = malloc(24);
	b = malloc(24);
	c = malloc(300);
	strcpy(a, "Hello malloc!");
	free(b);
	c = realloc(c, 2000);
	show_alloc_mem_ex();
	free(a);
	free(a);
	free(c);
	return (0);
}
