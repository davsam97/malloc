#ifndef ZONE_H
# define ZONE_H

# include "core/types.h"

size_t	align_up(size_t n, size_t align);
t_type	type_of(size_t size);
t_zone	*zone_new(t_type type, size_t total);
t_zone	*zone_new_small(t_type type);
void	zone_release(t_zone *zone);
t_zone	*zone_of(void *ptr);
int		zone_table_add(t_zone *zone);
void	zone_table_remove(t_zone *zone);

#endif
