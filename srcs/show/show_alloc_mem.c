#include "block/block.h"
#include "show/show.h"
#include "utils/utils.h"

void	put_zone_header(t_zone *zone)
{
	if (zone->type == TINY)
		ft_putstr_fd("TINY : ", 1);
	else if (zone->type == SMALL)
		ft_putstr_fd("SMALL : ", 1);
	else
		ft_putstr_fd("LARGE : ", 1);
	ft_puthex_fd((size_t)zone, 1);
}

void	put_range(t_block *block, size_t len)
{
	ft_puthex_fd((size_t)block_data(block), 1);
	ft_putstr_fd(" - ", 1);
	ft_puthex_fd((size_t)block_data(block) + len, 1);
}

static void	show_block(t_block *block)
{
	put_range(block, block->req);
	ft_putstr_fd(" : ", 1);
	ft_putnbr_fd(block->req, 1);
	ft_putstr_fd(" bytes\n", 1);
}

static size_t	show_zone(t_zone *zone)
{
	t_block	*block;
	size_t	total;

	block = zone->blocks;
	while (block && block->free)
		block = block->next;
	if (!block)
		return (0);
	put_zone_header(zone);
	ft_putstr_fd("\n", 1);
	total = 0;
	while (block)
	{
		if (!block->free)
		{
			show_block(block);
			total += block->req;
		}
		block = block->next;
	}
	return (total);
}

void	show_alloc_mem(void)
{
	size_t	i;
	size_t	total;

	pthread_mutex_lock(&g_mutex);
	total = 0;
	i = 0;
	while (i < g_heap.count)
	{
		total += show_zone(g_heap.zones[i]);
		i++;
	}
	ft_putstr_fd("Total : ", 1);
	ft_putnbr_fd(total, 1);
	ft_putstr_fd(" bytes\n", 1);
	pthread_mutex_unlock(&g_mutex);
}
