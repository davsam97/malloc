#include "malloc.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static int	check(int ok, char *name)
{
	if (ok)
		write(1, "OK   ", 5);
	else
		write(1, "FAIL ", 5);
	write(1, name, strlen(name));
	write(1, "\n", 1);
	return (!ok);
}

static int	sizes(void)
{
	static size_t	tab[] = {0, 1, 15, 16, 17, 128, 129, 1024, 1025, 70000};
	size_t			i;
	char			*ptr;
	int				ok;

	ok = 1;
	i = 0;
	while (i < sizeof(tab) / sizeof(*tab))
	{
		ptr = malloc(tab[i]);
		if (!ptr || (uintptr_t)ptr % 16)
			ok = 0;
		memset(ptr, 'x', tab[i]);
		free(ptr);
		i++;
	}
	return (check(ok, "malloc: tailles limites, alignement 16"));
}

static int	reallocs(void)
{
	char	*ptr;
	int		err;

	ptr = realloc(NULL, 10);
	err = check(ptr != NULL, "realloc(NULL, n) == malloc(n)");
	strcpy(ptr, "abcdefghi");
	ptr = realloc(ptr, 100);
	err += check(!strcmp(ptr, "abcdefghi"), "realloc TINY -> TINY");
	ptr = realloc(ptr, 600);
	err += check(!strcmp(ptr, "abcdefghi"), "realloc TINY -> SMALL");
	ptr = realloc(ptr, 5000);
	err += check(!strcmp(ptr, "abcdefghi"), "realloc SMALL -> LARGE");
	ptr = realloc(ptr, 4);
	err += check(!memcmp(ptr, "abcd", 4), "realloc LARGE -> TINY");
	free(ptr);
	return (err);
}

static int	invalid(void)
{
	char	stack[32];
	char	*ptr;
	int		err;

	free(NULL);
	free(stack);
	err = check(realloc(stack, 10) == NULL, "realloc pointeur de pile -> NULL");
	ptr = malloc(64);
	free(ptr + 8);
	err += check(realloc(ptr + 8, 10) == NULL, "realloc milieu -> NULL");
	free(ptr);
	free(ptr);
	err += check(realloc(ptr, 10) == NULL, "realloc bloc libere -> NULL");
	return (err);
}

int	main(void)
{
	int	err;

	err = sizes();
	err += reallocs();
	err += invalid();
	show_alloc_mem();
	return (err != 0);
}
