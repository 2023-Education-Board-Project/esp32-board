#include "lvgl_ui.h"
#include "ble_server.h"


static void	init(void)
{
	int	ret;
	
	/* init display (lvgl) */
	init_disp();

	/* init ble_server */
	ret = ble_server_init();
	assert(ret != 0);	
}

void	app_main(void)
{
	init();

	/* start ble_server */
	ble_server_start();	
}
