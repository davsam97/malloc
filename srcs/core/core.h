#ifndef CORE_H
# define CORE_H

# include "core/types.h"

void		heap_init(void);
void		*heap_alloc(size_t size, size_t align);
void		heap_free(t_loc *loc);
void		*heap_realloc(t_loc *loc, size_t size);
t_lookup	heap_lookup(void *ptr, t_loc *loc);

#endif
