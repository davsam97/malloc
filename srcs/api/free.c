#include "core/core.h"
#include "debug/debug.h"

void	free(void *ptr)
{
	t_loc		loc;
	t_lookup	status;

	if (!ptr)
		return ;
	pthread_mutex_lock(&g_mutex);
	status = heap_lookup(ptr, &loc);
	if (status == LOOKUP_OK)
	{
		event_record(OP_FREE, ptr, NULL, loc.block->req);
		heap_free(&loc);
	}
	else
		report_invalid("free", ptr, status);
	pthread_mutex_unlock(&g_mutex);
}
