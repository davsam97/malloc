#include "debug/debug.h"
#include "utils/utils.h"
#include <stdlib.h>

static const char	*op_name(t_op op)
{
	if (op == OP_MALLOC)
		return ("malloc(");
	if (op == OP_CALLOC)
		return ("calloc(");
	if (op == OP_MEMALIGN)
		return ("memalign(");
	if (op == OP_REALLOC)
		return ("realloc(");
	return ("free(");
}

void	event_print(t_event *event, int fd)
{
	ft_putstr_fd(op_name(event->op), fd);
	if (event->op == OP_FREE)
	{
		ft_puthex_fd((size_t)event->ptr, fd);
		ft_putstr_fd(") ", fd);
		ft_putnbr_fd(event->size, fd);
		ft_putstr_fd(" bytes\n", fd);
		return ;
	}
	if (event->op == OP_REALLOC)
	{
		ft_puthex_fd((size_t)event->old, fd);
		ft_putstr_fd(", ", fd);
	}
	ft_putnbr_fd(event->size, fd);
	ft_putstr_fd(") = ", fd);
	ft_puthex_fd((size_t)event->ptr, fd);
	ft_putstr_fd("\n", fd);
}

void	event_record(t_op op, void *ptr, void *old, size_t size)
{
	t_event	*event;

	event = &g_heap.history[g_heap.events % HISTORY_SIZE];
	event->op = op;
	event->ptr = ptr;
	event->old = old;
	event->size = size;
	g_heap.events++;
	if (g_heap.verbose)
	{
		ft_putstr_fd("[ft_malloc] ", 2);
		event_print(event, 2);
	}
}

void	report_invalid(const char *func, void *ptr, t_lookup status)
{
	if (g_heap.check == 0)
		return ;
	ft_putstr_fd("[ft_malloc] *** error in ", 2);
	ft_putstr_fd(func, 2);
	ft_putstr_fd("(", 2);
	ft_puthex_fd((size_t)ptr, 2);
	if (status == LOOKUP_FREED)
		ft_putstr_fd("): double free\n", 2);
	else if (status == LOOKUP_INSIDE)
		ft_putstr_fd("): pointer is not the start of a block\n", 2);
	else
		ft_putstr_fd("): pointer was not allocated by malloc\n", 2);
	if (g_heap.check >= 2)
		abort();
}
