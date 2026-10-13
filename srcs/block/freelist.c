#include "block/block.h"

static t_link	*link_of(t_block *block)
{
	return ((t_link *)block_data(block));
}

void	freelist_push(t_block *block, t_type type)
{
	t_link	*link;

	link = link_of(block);
	link->prev = NULL;
	link->next = g_heap.bins[type];
	if (link->next)
		link_of(link->next)->prev = block;
	g_heap.bins[type] = block;
}

void	freelist_remove(t_block *block, t_type type)
{
	t_link	*link;

	link = link_of(block);
	if (link->prev)
		link_of(link->prev)->next = link->next;
	else
		g_heap.bins[type] = link->next;
	if (link->next)
		link_of(link->next)->prev = link->prev;
}

t_block	*block_find_free(t_type type, size_t size)
{
	t_block	*block;

	block = g_heap.bins[type];
	while (block && block->size < size)
		block = link_of(block)->next;
	return (block);
}
