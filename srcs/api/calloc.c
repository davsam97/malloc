#include "api/api.h"
#include "core/core.h"
#include "utils/utils.h"
#include "zone/zone.h"
#include <stdlib.h>

// Fresh LARGE mappings are already zeroed
void	*calloc(size_t nmemb, size_t size)
{
	void	*ptr;
	size_t	total;

	if (size && nmemb > SIZE_MAX / size)
		return (NULL);
	total = nmemb * size;
	ptr = locked_alloc(total, ALIGNMENT, OP_CALLOC);
	if (ptr && (type_of(total) != LARGE || g_heap.opts & OPT_SCRIBBLE))
		ft_memset(ptr, 0, total);
	return (ptr);
}

void	*reallocarray(void *ptr, size_t nmemb, size_t size)
{
	if (size && nmemb > SIZE_MAX / size)
		return (NULL);
	return (realloc(ptr, nmemb * size));
}

size_t	malloc_usable_size(void *ptr)
{
	t_loc	loc;
	size_t	size;

	size = 0;
	pthread_mutex_lock(&g_mutex);
	if (heap_lookup(ptr, &loc) == LOOKUP_OK)
		size = loc.block->size;
	pthread_mutex_unlock(&g_mutex);
	return (size);
}
