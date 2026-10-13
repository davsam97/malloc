#include "block/block.h"
#include "zone/zone.h"

void	*block_data(t_block *block)
{
	return ((char *)block + sizeof(t_block));
}

// Returns 0 when the request is too large
size_t	payload_size(size_t size)
{
	if (size > SIZE_MAX / 2)
		return (0);
	if (size == 0)
		return (ALIGNMENT);
	return (align_up(size, ALIGNMENT));
}

// Splits only if the remainder can hold another block
void	block_split(t_block *block, size_t size, t_type type)
{
	t_block	*rest;

	if (block->size < size + sizeof(t_block) + ALIGNMENT)
		return ;
	rest = (t_block *)((char *)block_data(block) + size);
	rest->size = block->size - size - sizeof(t_block);
	rest->free = 1;
	rest->next = block->next;
	block->next = rest;
	block->size = size;
	freelist_push(rest, type);
	block_merge(rest, type);
}

void	block_merge(t_block *block, t_type type)
{
	while (block->next && block->next->free)
	{
		freelist_remove(block->next, type);
		block->size += sizeof(t_block) + block->next->size;
		block->next = block->next->next;
	}
}
