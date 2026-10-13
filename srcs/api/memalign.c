#include "api/api.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

// Replaced so that libc never hands out its own pointers
void	*memalign(size_t alignment, size_t size)
{
	if (alignment == 0 || (alignment & (alignment - 1)))
		return (NULL);
	if (alignment < ALIGNMENT)
		alignment = ALIGNMENT;
	return (locked_alloc(size, alignment, OP_MEMALIGN));
}

int	posix_memalign(void **memptr, size_t alignment, size_t size)
{
	void	*ptr;

	if (alignment % sizeof(void *) || (alignment & (alignment - 1)))
		return (EINVAL);
	ptr = memalign(alignment, size);
	if (!ptr)
		return (ENOMEM);
	*memptr = ptr;
	return (0);
}

void	*aligned_alloc(size_t alignment, size_t size)
{
	return (memalign(alignment, size));
}

void	*valloc(size_t size)
{
	return (memalign((size_t)sysconf(_SC_PAGESIZE), size));
}
