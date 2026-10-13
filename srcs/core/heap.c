#include "block/block.h"
#include "core/core.h"
#include "utils/utils.h"
#include "zone/zone.h"

static t_block	*alloc_large(size_t size, size_t align)
{
	size_t	offset;
	size_t	total;
	t_zone	*zone;

	offset = align_up(sizeof(t_zone) + sizeof(t_block), align)
		- sizeof(t_block);
	if (size > SIZE_MAX - offset - sizeof(t_block) - g_heap.page_size)
		return (NULL);
	total = align_up(offset + sizeof(t_block) + size, g_heap.page_size);
	zone = zone_new(LARGE, total, offset);
	if (!zone)
		return (NULL);
	return (zone->blocks);
}

static t_block	*alloc_small(t_type type, size_t size)
{
	t_block	*block;
	t_zone	*zone;

	block = block_find_free(type, size);
	if (block)
		freelist_remove(block, type);
	else
	{
		zone = zone_new(type, zone_capacity(type), sizeof(t_zone));
		if (!zone)
			return (NULL);
		block = zone->blocks;
	}
	block_split(block, size, type);
	return (block);
}

// Caller holds g_mutex; alignments above ALIGNMENT get their own mmap
void	*heap_alloc(size_t size, size_t align)
{
	t_block	*block;
	size_t	aligned;

	heap_init();
	aligned = payload_size(size);
	if (!aligned)
		return (NULL);
	if (type_of(size) == LARGE || align > ALIGNMENT)
		block = alloc_large(aligned, align);
	else
		block = alloc_small(type_of(size), aligned);
	if (!block)
		return (NULL);
	block->free = 0;
	block->req = size;
	if (g_heap.opts & OPT_SCRIBBLE)
		ft_memset(block_data(block), SCRIBBLE_ALLOC, block->req);
	return (block_data(block));
}

void	heap_free(t_loc *loc)
{
	t_block	*block;

	block = loc->block;
	if (loc->zone->type == LARGE)
	{
		zone_release(loc->zone);
		return ;
	}
	if (g_heap.opts & OPT_SCRIBBLE)
		ft_memset(block_data(block), SCRIBBLE_FREE, block->size);
	block->free = 1;
	freelist_push(block, loc->zone->type);
	block_merge(block, loc->zone->type);
	if (loc->prev && loc->prev->free)
		block_merge(loc->prev, loc->zone->type);
	if (zone_is_reclaimable(loc->zone))
	{
		freelist_remove(loc->zone->blocks, loc->zone->type);
		zone_release(loc->zone);
	}
}
