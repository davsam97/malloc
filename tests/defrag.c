#include "malloc.h"
#include <unistd.h>

static void	step(const char *msg)
{
	size_t	len;

	len = 0;
	while (msg[len])
		len++;
	write(1, "\n===== ", 7);
	write(1, msg, len);
	write(1, " =====\n", 7);
}

int	main(void)
{
	char	*a;
	char	*b;
	char	*c;
	char	*d;
	char	*e;

	a = malloc(16);
	b = malloc(16);
	c = malloc(16);
	d = malloc(16);
	step("1. A B C D alloues (16 octets chacun)");
	show_alloc_mem_ex();
	free(a);
	free(c);
	step("2. free(A) et free(C) : 2 trous separes par B");
	show_alloc_mem_ex();
	free(b);
	step("3. free(B) : A, B et C fusionnent en un seul bloc");
	show_alloc_mem_ex();
	e = malloc(100);
	step("4. malloc(100) : trop grand pour un trou de 16, rentre dans le bloc fusionne");
	show_alloc_mem_ex();
	if (e == a)
		write(1, "\n=> OK : malloc(100) a recu l'adresse de A\n", 43);
	free(d);
	free(e);
	return (0);
}
