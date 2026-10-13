#include "utils/utils.h"
#include "zone/zone.h"
#include <sys/mman.h>

// The table lives in its own mapping and doubles when full
static int	table_grow(void)
{
	t_zone	**table;
	size_t	capacity;

	capacity = g_heap.page_size / sizeof(t_zone *);
	if (g_heap.capacity)
		capacity = g_heap.capacity * 2;
	table = mmap(NULL, capacity * sizeof(t_zone *), PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (table == MAP_FAILED)
		return (0);
	if (g_heap.zones)
	{
		ft_memcpy(table, g_heap.zones, g_heap.count * sizeof(t_zone *));
		munmap(g_heap.zones, g_heap.capacity * sizeof(t_zone *));
	}
	g_heap.zones = table;
	g_heap.capacity = capacity;
	return (1);
}

// Number of zones starting at or below ptr
static size_t	zone_search(void *ptr)
{
	size_t	low;
	size_t	high;
	size_t	mid;

	low = 0;
	high = g_heap.count;
	while (low < high)
	{
		mid = low + (high - low) / 2;
		if ((char *)g_heap.zones[mid] <= (char *)ptr)
			low = mid + 1;
		else
			high = mid;
	}
	return (low);
}

int	zone_table_add(t_zone *zone)
{
	size_t	pos;
	size_t	i;

	if (g_heap.count == g_heap.capacity && !table_grow())
		return (0);
	pos = zone_search(zone);
	i = g_heap.count;
	while (i > pos)
	{
		g_heap.zones[i] = g_heap.zones[i - 1];
		i--;
	}
	g_heap.zones[pos] = zone;
	g_heap.count++;
	return (1);
}

void	zone_table_remove(t_zone *zone)
{
	size_t	pos;

	pos = zone_search(zone);
	if (pos == 0 || g_heap.zones[pos - 1] != zone)
		return ;
	while (pos < g_heap.count)
	{
		g_heap.zones[pos - 1] = g_heap.zones[pos];
		pos++;
	}
	g_heap.count--;
}

t_zone	*zone_of(void *ptr)
{
	size_t	pos;
	t_zone	*zone;

	pos = zone_search(ptr);
	if (pos == 0)
		return (NULL);
	zone = g_heap.zones[pos - 1];
	if ((char *)ptr < (char *)zone + zone->size)
		return (zone);
	return (NULL);
}
