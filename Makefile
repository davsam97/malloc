ifeq ($(HOSTTYPE),)
    HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME		= libft_malloc_$(HOSTTYPE).so
LINK		= libft_malloc.so

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -fPIC -fno-stack-protector
LDFLAGS		= -shared
LDLIBS		= -lpthread

INC_DIR		= includes
SRC_DIR		= srcs
OBJ_DIR		= build

SRCS		= core/globals.c			\
			  core/heap.c				\
			  api/malloc.c				\
			  api/free.c				\
			  api/realloc.c				\
			  api/calloc.c				\
			  api/memalign.c			\
			  zone/zone.c				\
			  zone/zone_list.c			\
			  block/block.c				\
			  block/freelist.c			\
			  show/show_alloc_mem.c		\
			  show/show_alloc_mem_ex.c	\
			  show/hexdump.c			\
			  debug/debug.c				\
			  utils/utils.c

OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))
DEPS		= $(OBJS:.o=.d)

all: $(LINK)

$(LINK): $(NAME)
	ln -sf $(NAME) $(LINK)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -I$(INC_DIR) -I$(SRC_DIR) -c $< -o $@

bonus: all

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME) $(LINK)

re: fclean all

-include $(DEPS)

.PHONY: all bonus clean fclean re
