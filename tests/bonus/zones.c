#include "malloc.h"

// 100 allocations of the largest TINY size and 100 of the largest SMALL size
// must each fit in a single pre-allocated zone
int	main(void)
{
	void	*tiny[100];
	void	*small[100];
	int		i;

	i = 0;
	while (i < 100)
	{
		tiny[i] = malloc(128);
		small[i] = malloc(1024);
		i++;
	}
	show_alloc_mem();
	i = 0;
	while (i < 100)
	{
		free(tiny[i]);
		free(small[i]);
		i++;
	}
	return (0);
}
