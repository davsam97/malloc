#include "core/core.h"
#include <stdlib.h>
#include <unistd.h>

t_heap			g_heap;
pthread_mutex_t	g_mutex = PTHREAD_MUTEX_INITIALIZER;

// Returns the first digit of the variable, 0 if unset or not a digit
static int	env_level(const char *name)
{
	char	*value;

	value = getenv(name);
	if (!value || value[0] < '1' || value[0] > '9')
		return (0);
	return (value[0] - '0');
}

void	heap_init(void)
{
	long	page_size;

	if (g_heap.page_size)
		return ;
	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0)
		page_size = 4096;
	g_heap.scribble = env_level("FT_MALLOC_SCRIBBLE");
	g_heap.verbose = env_level("FT_MALLOC_VERBOSE");
	g_heap.check = env_level("FT_MALLOC_CHECK");
	g_heap.page_size = (size_t)page_size;
}
