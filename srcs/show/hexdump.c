#include "show/show.h"
#include "utils/utils.h"
#include <unistd.h>

static void	put_byte_hex(unsigned char byte, int fd)
{
	char	buf[3];

	buf[0] = HEX_DIGITS[byte / 16];
	buf[1] = HEX_DIGITS[byte % 16];
	buf[2] = ' ';
	write(fd, buf, 3);
}

static void	put_offset(size_t offset, int fd)
{
	char	buf[8];
	int		i;

	buf[0] = ' ';
	buf[1] = ' ';
	buf[2] = ' ';
	buf[3] = ' ';
	i = 8;
	while (i > 4)
	{
		i--;
		buf[i] = HEX_DIGITS[offset % 16];
		offset /= 16;
	}
	write(fd, buf, 8);
	write(fd, "  ", 2);
}

static void	put_ascii(unsigned char *line, size_t len, int fd)
{
	char	buf[16];
	size_t	i;

	i = 0;
	while (i < len)
	{
		buf[i] = '.';
		if (line[i] >= 32 && line[i] < 127)
			buf[i] = line[i];
		i++;
	}
	write(fd, "|", 1);
	write(fd, buf, len);
	write(fd, "|\n", 2);
}

void	hexdump(void *addr, size_t len, int fd)
{
	unsigned char	*data;
	size_t			offset;
	size_t			i;

	data = addr;
	offset = 0;
	while (offset < len)
	{
		put_offset(offset, fd);
		i = 0;
		while (i < 16)
		{
			if (offset + i < len)
				put_byte_hex(data[offset + i], fd);
			else
				write(fd, "   ", 3);
			i++;
		}
		i = 16;
		if (len - offset < 16)
			i = len - offset;
		put_ascii(data + offset, i, fd);
		offset += 16;
	}
}
