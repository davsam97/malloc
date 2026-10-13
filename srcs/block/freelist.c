#include "block/block.h"

static t_link	*link_of(t_block *block)
{
	return ((t_link *)block_data(block));
}

static size_t	bin_of(size_t size)
{
	if (size / ALIGNMENT >= BIN_COUNT)
		return (BIN_COUNT - 1);
	return (size / ALIGNMENT);
}

// A block must not change size while it is on a list
void	freelist_push(t_block *block, t_type type)
{
	t_link	*link;
	t_block	**bin;

	bin = &g_heap.bins[type][bin_of(block->size)];
	link = link_of(block);
	link->prev = NULL;
	link->next = *bin;
	if (link->next)
		link_of(link->next)->prev = block;
	*bin = block;
}

void	freelist_remove(t_block *block, t_type type)
{
	t_link	*link;

	link = link_of(block);
	if (link->prev)
		link_of(link->prev)->next = link->next;
	else
		g_heap.bins[type][bin_of(block->size)] = link->next;
	if (link->next)
		link_of(link->next)->prev = link->prev;
}

// Every block of an exact bin fits: only the last bin needs a search
t_block	*block_find_free(t_type type, size_t size)
{
	size_t	i;
	t_block	*block;

	i = bin_of(size);
	while (i < BIN_COUNT - 1 && !g_heap.bins[type][i])
		i++;
	block = g_heap.bins[type][i];
	while (block && block->size < size)
		block = link_of(block)->next;
	return (block);
}
