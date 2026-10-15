#include "api/api.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

// Divides by 2 until odd: only powers of two end up at exactly 1
static int	is_power_of_two(size_t n)
{
	if (n == 0)
		return (0);
	while (n % 2 == 0)
		n /= 2;
	return (n == 1);
}

// Replaced so that libc never hands out its own pointers
void	*memalign(size_t alignment, size_t size)
{
	if (!is_power_of_two(alignment))
		return (NULL);
	if (alignment < ALIGNMENT)
		alignment = ALIGNMENT;
	return (locked_alloc(size, alignment, OP_MEMALIGN));
}

int	posix_memalign(void **memptr, size_t alignment, size_t size)
{
	void	*ptr;

	if (alignment % sizeof(void *) || !is_power_of_two(alignment))
		return (EINVAL);
	ptr = memalign(alignment, size);
	if (!ptr)
		return (EINVAL);
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
