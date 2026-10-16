#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define FORKS 300

// fork() while other threads use malloc: the child must be able to malloc.
// Returns the number of children stuck for more than 200 ms
static volatile int	g_stop;

static void	*hammer(void *arg)
{
	unsigned		x;
	char *volatile	p;

	x = (unsigned)(long)arg * 2654435761u + 1;
	while (!g_stop)
	{
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		p = malloc(x % 900 + 1);
		p[0] = 1;
		free(p);
	}
	return (NULL);
}

static int	child_hangs(void)
{
	pid_t			pid;
	int				status;
	int				waited;
	char *volatile	p;

	pid = fork();
	if (pid == 0)
	{
		p = malloc(100);
		p[0] = 1;
		free(p);
		_exit(0);
	}
	waited = 0;
	while (waitpid(pid, &status, WNOHANG) == 0 && waited < 200)
	{
		usleep(1000);
		waited++;
	}
	if (waited < 200)
		return (0);
	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
	return (1);
}

int	main(void)
{
	pthread_t	threads[4];
	long		i;
	int			hung;

	i = 0;
	while (i < 4)
	{
		pthread_create(&threads[i], NULL, hammer, (void *)(i + 1));
		i++;
	}
	hung = 0;
	i = 0;
	while (i++ < FORKS)
		hung += child_hangs();
	g_stop = 1;
	i = 0;
	while (i < 4)
		pthread_join(threads[i++], NULL);
	printf("%d/%d children deadlocked\n", hung, FORKS);
	return (hung != 0);
}
