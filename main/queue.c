#include <stdio.h>
#include <assert.h>

#include "queue.h"

void	initqueue(t_queue *queue)
{
	queue->size = 0;
	queue->head.val = NULL;
	queue->head.next = NULL;
	queue->tail = &(queue->head);
}

void	enqueue(t_queue *queue, void *val)
{
	t_node	*tmp;
	
	//allocate
	tmp = (t_node *)heap_caps_malloc(sizeof(t_node), MALLOC_CAP_32BIT);
	assert(tmp);
	//init
	tmp->val = val;
	tmp->next = NULL;
	
	queue->tail->next = (void *)tmp;
	queue->tail = (void *)tmp;

	queue->size += 1;
}

void	dequeue(t_queue *queue, void (*del)(void *))
{
	t_node	*tmp;
	
	assert(queue->size);
	tmp = queue->head.next;
	del(tmp->val);
	queue->head.next = queue->head.next->next;
	heap_caps_free(tmp);
	queue->size -= 1;
	if (!queue->size)
		queue->tail = &(queue->head);
}

void	*get_item(t_queue *queue, int index)
{
	t_node	*tmp;

	assert(index < queue->size);
	tmp = queue->head.next;
	for (int i = 0; i < index; i++)
		tmp = tmp->next;
	return tmp->val;
}

void	delqueue(t_queue *queue, void (*del)(void *))
{
	while (queue->size)
		dequeue(queue, del);
}
