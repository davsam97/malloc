#ifndef TYPES_H
# define TYPES_H

# include "malloc.h"
# include <pthread.h>
# include <stddef.h>
# include <stdint.h>

# define ALIGNMENT 16
# define TINY_MAX 128
# define SMALL_MAX 1024
# define MIN_ALLOCS 100

# define HISTORY_SIZE 64

# define OPT_SCRIBBLE 1
# define OPT_VERBOSE 2
# define OPT_CHECK 4
# define OPT_ABORT 8

# define SCRIBBLE_ALLOC 0xAA
# define SCRIBBLE_FREE 0x55

typedef enum e_type
{
	TINY,
	SMALL,
	LARGE
}						t_type;

typedef enum e_op
{
	OP_MALLOC,
	OP_CALLOC,
	OP_MEMALIGN,
	OP_REALLOC,
	OP_FREE
}						t_op;

typedef enum e_lookup
{
	LOOKUP_NONE,
	LOOKUP_OK,
	LOOKUP_FREED,
	LOOKUP_INSIDE
}						t_lookup;

typedef struct s_block
{
	size_t				size;
	size_t				req;
	struct s_block		*next;
	size_t				free;
}						t_block;

// Free-list links are stored in the payload of free blocks
typedef struct s_link
{
	struct s_block		*next;
	struct s_block		*prev;
}						t_link;

typedef struct s_zone
{
	struct s_zone		*next;
	size_t				size;
	t_type				type;
	t_block				*blocks;
}						t_zone;

typedef struct s_loc
{
	t_zone				*zone;
	t_block				*block;
	t_block				*prev;
}						t_loc;

typedef struct s_event
{
	t_op				op;
	void				*ptr;
	void				*old;
	size_t				size;
}						t_event;

typedef struct s_heap
{
	t_zone				*zones;
	t_block				*bins[2];
	size_t				page_size;
	int					opts;
	size_t				events;
	t_event				history[HISTORY_SIZE];
}						t_heap;

extern t_heap			g_heap;
extern pthread_mutex_t	g_mutex;

#endif
