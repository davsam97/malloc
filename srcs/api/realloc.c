#include "block/block.h"
#include "core/core.h"
#include "debug/debug.h"
#include "utils/utils.h"
#include "zone/zone.h"

static int	resize_in_place(t_loc *loc, size_t size, size_t aligned)
{
	t_block	*block;
	t_block	*next;

	block = loc->block;
	next = block->next;
	if (loc->zone->type == LARGE)
		return (type_of(size) == LARGE && aligned <= block->size);
	if (aligned > block->size && type_of(size) == loc->zone->type && next
		&& next->free && block->size + sizeof(t_block) + next->size >= aligned)
		block_merge(block, loc->zone->type);
	if (aligned > block->size)
		return (0);
	block_split(block, aligned, loc->zone->type);
	return (1);
}

// heap_alloc may split loc->prev: the loop walks to the new neighbour
void	*heap_realloc(t_loc *loc, size_t size)
{
	void	*old;
	void	*new;
	size_t	old_req;

	if (!payload_size(size))
		return (NULL);
	old = block_data(loc->block);
	old_req = loc->block->req;
	if (resize_in_place(loc, size, payload_size(size)))
	{
		if (g_heap.scribble && size > old_req)
			ft_memset((char *)old + old_req, SCRIBBLE_ALLOC, size - old_req);
		loc->block->req = size;
		return (old);
	}
	new = heap_alloc(size, ALIGNMENT);
	if (!new)
		return (NULL);
	if (old_req > size)
		old_req = size;
	ft_memcpy(new, old, old_req);
	while (loc->prev && loc->prev->next != loc->block)
		loc->prev = loc->prev->next;
	heap_free(loc);
	return (new);
}

void	*realloc(void *ptr, size_t size)
{
	void		*new;
	t_loc		loc;
	t_lookup	status;

	if (!ptr)
		return (malloc(size));
	pthread_mutex_lock(&g_mutex);
	new = NULL;
	status = heap_lookup(ptr, &loc);
	if (status == LOOKUP_OK)
		new = heap_realloc(&loc, size);
	else
		report_invalid("realloc", ptr, status);
	event_record(OP_REALLOC, new, ptr, size);
	pthread_mutex_unlock(&g_mutex);
	return (new);
}
