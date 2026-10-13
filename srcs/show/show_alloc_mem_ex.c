#include "block/block.h"
#include "debug/debug.h"
#include "show/show.h"
#include "utils/utils.h"

static void	show_block_ex(t_block *block, size_t *used, size_t *unused)
{
	size_t	len;

	put_range(block, block->size);
	if (block->free)
	{
		ft_putstr_fd(" : free (", 1);
		ft_putnbr_fd(block->size, 1);
		ft_putstr_fd(" bytes)\n", 1);
		*unused += block->size;
		return ;
	}
	ft_putstr_fd(" : ", 1);
	ft_putnbr_fd(block->req, 1);
	ft_putstr_fd(" bytes used\n", 1);
	*used += block->req;
	len = block->req;
	if (len > DUMP_MAX)
		len = DUMP_MAX;
	hexdump(block_data(block), len, 1);
	if (block->req > DUMP_MAX)
		ft_putstr_fd("    ...\n", 1);
}

static void	show_zone_ex(t_zone *zone, size_t *used, size_t *unused)
{
	t_block	*block;

	put_zone_header(zone);
	ft_putstr_fd(" (", 1);
	ft_putnbr_fd(zone->size, 1);
	ft_putstr_fd(" bytes mapped)\n", 1);
	block = zone->blocks;
	while (block)
	{
		show_block_ex(block, used, unused);
		block = block->next;
	}
}

static void	show_history(void)
{
	size_t	i;

	i = 0;
	if (g_heap.events > HISTORY_SIZE)
		i = g_heap.events - HISTORY_SIZE;
	ft_putstr_fd("History (", 1);
	ft_putnbr_fd(g_heap.events, 1);
	ft_putstr_fd(" operations, last ", 1);
	ft_putnbr_fd(g_heap.events - i, 1);
	ft_putstr_fd(" shown) :\n", 1);
	while (i < g_heap.events)
	{
		ft_putstr_fd("  #", 1);
		ft_putnbr_fd(i, 1);
		ft_putstr_fd(" ", 1);
		event_print(&g_heap.history[i % HISTORY_SIZE], 1);
		i++;
	}
}

void	show_alloc_mem_ex(void)
{
	t_zone	*zone;
	size_t	used;
	size_t	unused;
	size_t	zones;

	pthread_mutex_lock(&g_mutex);
	used = 0;
	unused = 0;
	zones = 0;
	zone = g_heap.zones;
	while (zone)
	{
		show_zone_ex(zone, &used, &unused);
		zones++;
		zone = zone->next;
	}
	ft_putstr_fd("Total : ", 1);
	ft_putnbr_fd(used, 1);
	ft_putstr_fd(" bytes used, ", 1);
	ft_putnbr_fd(unused, 1);
	ft_putstr_fd(" bytes free, ", 1);
	ft_putnbr_fd(zones, 1);
	ft_putstr_fd(" zones\n", 1);
	show_history();
	pthread_mutex_unlock(&g_mutex);
}
