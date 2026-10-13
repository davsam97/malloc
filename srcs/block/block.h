#ifndef BLOCK_H
# define BLOCK_H

# include "core/types.h"

void	*block_data(t_block *block);
size_t	payload_size(size_t size);
void	block_split(t_block *block, size_t size, t_type type);
void	block_merge(t_block *block, t_type type);
void	freelist_push(t_block *block, t_type type);
void	freelist_remove(t_block *block, t_type type);
t_block	*block_find_free(t_type type, size_t size);

#endif
