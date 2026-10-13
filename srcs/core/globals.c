#include "core/core.h"
#include <stdlib.h>
#include <unistd.h>

t_heap			g_heap;
pthread_mutex_t	g_mutex = PTHREAD_MUTEX_INITIALIZER;

// FT_MALLOC_SCRIBBLE, FT_MALLOC_VERBOSE, FT_MALLOC_CHECK (1: report, 2: abort)
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
	int		check;

	if (g_heap.page_size)
		return ;
	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0)
		page_size = 4096;
	if (env_level("FT_MALLOC_SCRIBBLE"))
		g_heap.opts |= OPT_SCRIBBLE;
	if (env_level("FT_MALLOC_VERBOSE"))
		g_heap.opts |= OPT_VERBOSE;
	check = env_level("FT_MALLOC_CHECK");
	if (check >= 1)
		g_heap.opts |= OPT_CHECK;
	if (check >= 2)
		g_heap.opts |= OPT_ABORT;
	g_heap.page_size = (size_t)page_size;
}
