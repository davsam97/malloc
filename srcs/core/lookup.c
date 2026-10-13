#include "block/block.h"
#include "core/core.h"
#include "zone/zone.h"

// A freed block merged into its neighbour is reported as LOOKUP_INSIDE
t_lookup	heap_lookup(void *ptr, t_loc *loc)
{
	t_block	*block;
	t_block	*prev;

	if (!ptr)
		return (LOOKUP_NONE);
	heap_init();
	loc->zone = zone_of(ptr);
	if (!loc->zone)
		return (LOOKUP_NONE);
	prev = NULL;
	block = loc->zone->blocks;
	while (block && (char *)block_data(block) <= (char *)ptr)
	{
		if (block_data(block) == ptr)
		{
			if (block->free)
				return (LOOKUP_FREED);
			loc->block = block;
			loc->prev = prev;
			return (LOOKUP_OK);
		}
		prev = block;
		block = block->next;
	}
	return (LOOKUP_INSIDE);
}
