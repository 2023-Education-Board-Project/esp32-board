
#include <inttypes.h>

#include "lvgl_ui.h"
#include "block.h"

static void	display_func(char *data);

void	mapping_block_task(uint8_t func, char *data)
{
	switch (func)
	{
		case 0x81:
			display_func(data);
			break;
		default:
			;
	}
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
