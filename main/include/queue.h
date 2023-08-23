#ifndef QUEUE_H
# define QUEUE_H

/*
# include "freertos/FreeRTOS.h"
# include "freertos/semphr.h"
*/

typedef struct s_node
{
	void	*val;
	struct s_node	*next;
}	t_node;

typedef struct s_queue
{
	t_node	head;
	t_node	*tail;
	int	size;
}	t_queue;

void	init(t_queue *queue);
void	enqueue(t_queue *queue, void *val);
void	*dequeue(t_queue *queue);
void	*get_item(t_queue *queue, int index);
void	delqueue(t_queue *queue);

#endif
