#include "block/block.h"
#include "core/core.h"
#include "zone/zone.h"
#include <sys/mman.h>

void	zone_release(t_zone *zone)
{
	t_zone	**cur;

	cur = &g_heap.zones;
	while (*cur && *cur != zone)
		cur = &(*cur)->next;
	if (*cur)
		*cur = zone->next;
	munmap(zone, zone->size);
}

// Keeps the last zone of a type to avoid mmap/munmap ping-pong
int	zone_is_reclaimable(t_zone *zone)
{
	t_zone	*cur;

	if (!zone->blocks->free || zone->blocks->next)
		return (0);
	cur = g_heap.zones;
	while (cur)
	{
		if (cur != zone && cur->type == zone->type)
			return (1);
		cur = cur->next;
	}
	return (0);
}

static t_zone	*zone_of(void *ptr)
{
	t_zone	*zone;

	zone = g_heap.zones;
	while (zone && (char *)zone < (char *)ptr)
	{
		if ((char *)ptr < (char *)zone + zone->size)
			return (zone);
		zone = zone->next;
	}
	return (NULL);
}

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
