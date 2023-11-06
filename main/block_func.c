
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "lvgl_ui.h"
#include "block.h"



static void task_init(uint8_t *data);
static void task_behaviour(void *param);
static void execute_func(uint8_t *data);
static void loop_func(uint8_t *data);
static void wait_func(uint8_t *data);
static void	display_func(char *data);

void	mapping_block_task(uint8_t *func)
{
	switch (func[0])
	{
		case 0x81:
			task_init(&(func[1]));
			break;
		default:
			;
	}
}

static void task_init(uint8_t *data)
{
	uint8_t	*func;
	size_t	len;

	len = strlen((char *)data) + 1;
	
	func = heap_caps_malloc(sizeof(uint8_t) * len, MALLOC_CAP_8BIT);
	assert(func != 0);
	memset(func, 0, len);

	strcpy((char *)func, (char *)data);
	//less priority than ble notify
	xTaskCreate(task_behaviour, "task_behaviour", 4096, (void *)func, tskIDLE_PRIORITY - 1, NULL);
}

static void task_behaviour(void *param)
{
	uint8_t	*data = (uint8_t *)param;
	
	execute_func(data);
	
	heap_caps_free(data);
	vTaskDelete(NULL);
}

static void execute_func(uint8_t *data)
{
	switch (data[0])
	{
		case 0xC1://display
			display_func((char *)(&(data[1])));
			break;
		case 0xC2://loop
			loop_func(&(data[1]));
			break;
		case 0xC3://wait
			wait_func(&(data[1]));
			break;
		default:
			print_text_lcd("%X\n", data[0]);
	}
}

static void loop_func(uint8_t *data)
{
	uint8_t	n;
	uint8_t	*func;
	
	n = data[0];
	func = &(data[1]);
	
	for (uint8_t i = 0; i < n; i++)
	{
		execute_func(func);
	}
}

static void wait_func(uint8_t *data)
{
	uint8_t	n;
	uint8_t	*func;
	
	n = data[0];
	func = &(data[1]);

	vTaskDelay(n * 1000 /  portTICK_PERIOD_MS);
	execute_func(func);
}

static void	display_func(char *data)
{
	static int	i = 0;
	if (!strlen(data))
		print_text_lcd("%s", data);
	else
		print_text_lcd("%s:%d\n", data, i);
	i++;
}

/**
 * waiting and signal
*/
