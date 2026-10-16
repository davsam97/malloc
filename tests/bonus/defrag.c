#include "malloc.h"

// Frees A and C, then B between them: the three blocks must merge into one
// free block, so a request of their total size reuses the address of A
int	main(int argc, char **argv)
{
	char	*a;
	char	*b;
	char	*c;
	char	*d;
	char	*big;

	(void)argv;
	a = malloc(200);
	b = malloc(200);
	c = malloc(200);
	d = malloc(200);
	free(a);
	free(c);
	if (argc > 1)
		show_alloc_mem_ex();
	free(b);
	if (argc > 1)
		show_alloc_mem_ex();
	big = malloc(600);
	if (big != a)
		return (1);
	free(big);
	free(d);
	return (0);
}
