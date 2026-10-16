#include "malloc.h"

// Run with FT_MALLOC_SCRIBBLE=1: new memory is filled with 0xAA, freed
// memory with 0x55. Byte 32 is read because a free block keeps its free
// list links in its first 16 bytes
int	main(void)
{
	unsigned char	*p;
	unsigned char	after_malloc;
	unsigned char	after_free;

	p = malloc(64);
	after_malloc = p[32];
	free(p);
	after_free = p[32];
	if (after_malloc != 0xAA)
		return (1);
	if (after_free != 0x55)
		return (2);
	return (0);
}
