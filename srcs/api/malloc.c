#include "api/api.h"
#include "core/core.h"
#include "debug/debug.h"

void	*locked_alloc(size_t size, size_t align, t_op op)
{
	void	*ptr;

	pthread_mutex_lock(&g_mutex);
	ptr = heap_alloc(size, align);
	event_record(op, ptr, NULL, size);
	pthread_mutex_unlock(&g_mutex);
	return (ptr);
}

void	*malloc(size_t size)
{
	return (locked_alloc(size, ALIGNMENT, OP_MALLOC));
}
