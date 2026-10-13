#include "zone/zone.h"
#include <sys/mman.h>

size_t	align_up(size_t n, size_t align)
{
	size_t	remainder;
	size_t	padding;

	remainder = n % align;
	if (remainder == 0)
		return (n);
	padding = align - remainder;
	return (n + padding);
}

t_type	type_of(size_t size)
{
	if (size <= TINY_MAX)
		return (TINY);
	if (size <= SMALL_MAX)
		return (SMALL);
	return (LARGE);
}

size_t	zone_capacity(t_type type)
{
	size_t	max;

	max = TINY_MAX;
	if (type == SMALL)
		max = SMALL_MAX;
	return (align_up(sizeof(t_zone) + MIN_ALLOCS * (sizeof(t_block) + max),
			g_heap.page_size));
}

// Sorted by address for show_alloc_mem
static void	zone_insert(t_zone *zone)
{
	t_zone	**cur;

	cur = &g_heap.zones;
	while (*cur && (char *)*cur < (char *)zone)
		cur = &(*cur)->next;
	zone->next = *cur;
	*cur = zone;
}

// The caller takes the block, so it is not put on the free list
t_zone	*zone_new(t_type type, size_t total, size_t offset)
{
	t_zone	*zone;
	t_block	*block;

	zone = mmap(NULL, total, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (zone == MAP_FAILED)
		return (NULL);
	zone->type = type;
	zone->size = total;
	block = (t_block *)((char *)zone + offset);
	block->size = total - offset - sizeof(t_block);
	block->free = 1;
	block->next = NULL;
	zone->blocks = block;
	zone_insert(zone);
	return (zone);
}
