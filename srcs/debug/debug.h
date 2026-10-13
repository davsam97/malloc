#ifndef DEBUG_H
# define DEBUG_H

# include "core/types.h"

void	event_record(t_op op, void *ptr, void *old, size_t size);
void	event_print(t_event *event, int fd);
void	report_invalid(const char *func, void *ptr, t_lookup status);

#endif
