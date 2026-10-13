#ifndef SHOW_H
# define SHOW_H

# include "core/types.h"

# define DUMP_MAX 64

void	put_zone_header(t_zone *zone);
void	put_range(t_block *block, size_t len);
void	hexdump(void *addr, size_t len, int fd);

#endif
