#include <stdio.h>
#include <assert.h>

#include "queue.h"

void	initqueue(t_queue *queue)
{
	queue->size = 0;
	queue->head.val = NULL;
	queue->head.next = NULL;
	queue->tail = &(queue->head);
	queue->mutex = xSemaphoreCreateMutex();
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
	
	xSemaphoreTake(queue->mutex, 1000 / portTICK_PERIOD_MS);

	queue->tail->next = (void *)tmp;
	queue->tail = (void *)tmp;

	queue->size += 1;
	xSemaphoreGive(queue->mutex);
}

void	dequeue(t_queue *queue, void (*del)(void *))
{
	t_node	*tmp;
	
	assert(queue->size);
	xSemaphoreTake(queue->mutex, 1000 / portTICK_PERIOD_MS);
	
	tmp = queue->head.next;
	queue->head.next = queue->head.next->next;
	queue->size -= 1;
	if (!queue->size)
		queue->tail = &(queue->head);

	xSemaphoreGive(queue->mutex);
	
	del(tmp->val);
	heap_caps_free(tmp);
}

void	*get_item(t_queue *queue, int index)
{
	t_node	*tmp;

	assert(index < queue->size);
	xSemaphoreTake(queue->mutex, 1000 / portTICK_PERIOD_MS);
	
	tmp = queue->head.next;
	for (int i = 0; i < index; i++)
		tmp = tmp->next;
	
	xSemaphoreGive(queue->mutex);
	return tmp->val;
}

void	delqueue(t_queue *queue, void (*del)(void *))
{
	int	size;

	xSemaphoreTake(queue->mutex, 1000 / portTICK_PERIOD_MS);
	size = queue->size;
	xSemaphoreGive(queue->mutex);
	
	while (size--)
		dequeue(queue, del);
	vSemaphoreDelete(queue->mutex);
}
