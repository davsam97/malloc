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

t_zone	*zone_new(t_type type, size_t total)
{
	t_zone	*zone;

	zone = mmap(NULL, total, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (zone == MAP_FAILED)
		return (NULL);
	zone->type = type;
	zone->size = total;
	zone->blocks = NULL;
	if (!zone_table_add(zone))
	{
		munmap(zone, total);
		return (NULL);
	}
	return (zone);
}

// Holds MIN_ALLOCS blocks of the type's largest size, rounded to pages.
// The header is aligned so every payload is. The caller takes the single
// free block, so it is not put on the free list.
t_zone	*zone_new_small(t_type type)
{
	size_t	header;
	size_t	max;
	size_t	total;
	t_zone	*zone;
	t_block	*block;

	header = align_up(sizeof(t_zone), ALIGNMENT);
	max = TINY_MAX;
	if (type == SMALL)
		max = SMALL_MAX;
	total = align_up(header + MIN_ALLOCS * (sizeof(t_block) + max),
			g_heap.page_size);
	zone = zone_new(type, total);
	if (!zone)
		return (NULL);
	block = (t_block *)((char *)zone + header);
	block->size = total - header - sizeof(t_block);
	block->free = 1;
	block->next = NULL;
	zone->blocks = block;
	return (zone);
}

void	zone_release(t_zone *zone)
{
	zone_table_remove(zone);
	munmap(zone, zone->size);
}
