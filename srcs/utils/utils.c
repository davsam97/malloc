#include "utils/utils.h"
#include <unistd.h>

void	*ft_memset(void *dst, int c, size_t len)
{
	unsigned char	*d;
	size_t			i;

	d = dst;
	i = 0;
	while (i < len)
	{
		d[i] = (unsigned char)c;
		i++;
	}
	return (dst);
}

void	*ft_memcpy(void *dst, const void *src, size_t len)
{
	unsigned char		*d;
	const unsigned char	*s;
	size_t				i;

	d = dst;
	s = src;
	i = 0;
	while (i < len)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}

void	ft_putstr_fd(const char *s, int fd)
{
	size_t	len;

	len = 0;
	while (s[len])
		len++;
	write(fd, s, len);
}

void	ft_putnbr_fd(size_t n, int fd)
{
	char	buf[20];
	int		i;

	i = 20;
	while (n || i == 20)
	{
		i--;
		buf[i] = '0' + n % 10;
		n /= 10;
	}
	write(fd, buf + i, 20 - i);
}

void	ft_puthex_fd(size_t n, int fd)
{
	char	buf[18];
	int		i;

	i = 18;
	while (n || i == 18)
	{
		i--;
		buf[i] = HEX_DIGITS[n % 16];
		n /= 16;
	}
	i -= 2;
	buf[i] = '0';
	buf[i + 1] = 'x';
	write(fd, buf + i, 18 - i);
}
