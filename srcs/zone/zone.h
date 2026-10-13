#ifndef ZONE_H
# define ZONE_H

# include "core/types.h"

size_t	align_up(size_t n, size_t align);
t_type	type_of(size_t size);
size_t	zone_capacity(t_type type);
t_zone	*zone_new(t_type type, size_t total, size_t offset);
void	zone_release(t_zone *zone);
int		zone_is_reclaimable(t_zone *zone);

#endif
