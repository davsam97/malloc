#include "malloc.h"
#include <pthread.h>

#define THREADS 8
#define ROUNDS 100000

void		__libc_freeres(void);

static void	*work(void *arg)
{
	int		i;
	char	*ptr;

	(void)arg;
	i = 0;
	while (i < ROUNDS)
	{
		ptr = malloc(i % 2000 + 1);
		ptr[0] = 42;
		ptr = realloc(ptr, i % 3000 + 1);
		free(ptr);
		if (i % 1000 == 0)
			show_alloc_mem();
		i++;
	}
	return (NULL);
}

// __libc_freeres vide le cache de piles de la glibc (et leurs DTV)
int	main(void)
{
	pthread_t	threads[THREADS];
	int			i;

	i = 0;
	while (i < THREADS)
	{
		pthread_create(&threads[i], NULL, work, NULL);
		i++;
	}
	i = 0;
	while (i < THREADS)
	{
		pthread_join(threads[i], NULL);
		i++;
	}
	__libc_freeres();
	show_alloc_mem();
	return (0);
}
