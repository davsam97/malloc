#include "block/block.h"
#include "core/core.h"
#include "utils/utils.h"
#include "zone/zone.h"

// mmap only aligns on pages: align - ALIGNMENT bytes of slack let the
// payload be shifted to a larger alignment
static t_block	*alloc_large(size_t size, size_t align)
{
	size_t	header;
	size_t	total;
	size_t	data;
	t_zone	*zone;
	t_block	*block;

	if (align > SIZE_MAX / 4)
		return (NULL);
	header = align_up(sizeof(t_zone) + sizeof(t_block), ALIGNMENT)
		+ align - ALIGNMENT;
	if (size > SIZE_MAX - header - g_heap.page_size)
		return (NULL);
	total = align_up(header + size, g_heap.page_size);
	zone = zone_new(LARGE, total);
	if (!zone)
		return (NULL);
	data = align_up((size_t)zone + sizeof(t_zone) + sizeof(t_block), align);
	block = (t_block *)(data - sizeof(t_block));
	block->size = (size_t)zone + total - data;
	block->next = NULL;
	zone->blocks = block;
	return (block);
}

static t_block	*alloc_small(t_type type, size_t size)
{
	t_block	*block;
	t_zone	*zone;

	block = block_find_free(type, size);
	if (block)
	{
		if (g_heap.spare[type] && g_heap.spare[type]->blocks == block)
			g_heap.spare[type] = NULL;
		freelist_remove(block, type);
	}
	else
	{
		zone = zone_new_small(type);
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

// One empty zone per type is kept to avoid mmap/munmap ping-pong
static void	release_if_empty(t_zone *zone)
{
	t_zone	**spare;

	if (!zone->blocks->free || zone->blocks->next)
		return ;
	spare = &g_heap.spare[zone->type];
	if (!*spare || *spare == zone)
	{
		*spare = zone;
		return ;
	}
	freelist_remove(zone->blocks, zone->type);
	zone_release(zone);
}

// Merges with the free neighbours on both sides, then lists the result once
void	heap_free(t_loc *loc)
{
	t_block	*block;
	t_type	type;

	if (loc->zone->type == LARGE)
	{
		zone_release(loc->zone);
		return ;
	}
	block = loc->block;
	type = loc->zone->type;
	if (g_heap.opts & OPT_SCRIBBLE)
		ft_memset(block_data(block), SCRIBBLE_FREE, block->size);
	block->free = 1;
	block_merge(block, type);
	if (loc->prev && loc->prev->free)
	{
		freelist_remove(loc->prev, type);
		loc->prev->size += sizeof(t_block) + block->size;
		loc->prev->next = block->next;
		block = loc->prev;
	}
	freelist_push(block, type);
	release_if_empty(loc->zone);
}
