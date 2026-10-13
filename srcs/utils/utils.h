#ifndef UTILS_H
# define UTILS_H

# include <stddef.h>

# define HEX_DIGITS "0123456789ABCDEF"

void	*ft_memset(void *dst, int c, size_t len);
void	*ft_memcpy(void *dst, const void *src, size_t len);
void	ft_putstr_fd(const char *s, int fd);
void	ft_putnbr_fd(size_t n, int fd);
void	ft_puthex_fd(size_t n, int fd);

#endif
