#include "lvgl_ui.h"

lv_obj_t	*labels[3];
static t_queue	g_text_queue;

static lv_disp_t	*disp;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	lv_disp_t	*disp = (lv_disp_t *)user_ctx;
	lvgl_port_flush_ready(disp);
	return false;
}

static i2c_config_t	init_i2c_bus(void)
{
	i2c_config_t	i2c_conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = PIN_NUM_SDA,
		.scl_io_num = PIN_NUM_SCL,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = LCD_PIXEL_CLOCK_HZ,
	};

	ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));
	return i2c_conf;
}

static esp_lcd_panel_io_i2c_config_t	install_panel_io(esp_lcd_panel_io_handle_t *io_handle)
{
	*io_handle = NULL;
	
	esp_lcd_panel_io_i2c_config_t	io_config = {
		.dev_addr = I2C_HW_ADDR,
		.control_phase_bytes = 1,
		.lcd_cmd_bits = LCD_CMD_BITS,
		.lcd_param_bits = LCD_PARAM_BITS,
#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
		.dc_bit_offset = 6,
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
		.dc_bit_offset = 0,
		.flags = {
			.disable_control_phase = 1,
		}
#endif
	};
	
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, io_handle));
	
	return io_config;
}

static esp_lcd_panel_dev_config_t	install_ssd1306_panel_driver(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t *panel_handle)
{
	*panel_handle = NULL;

	esp_lcd_panel_dev_config_t	panel_config = {
		.bits_per_pixel = 1,
		.reset_gpio_num = PIN_NUM_RST,
	};

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
	ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, panel_handle));
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
	ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, panel_handle));
#endif
	return panel_config;
}

void	init_disp(void)
{
	//init i2c, panel
	i2c_config_t	i2c_conf = init_i2c_bus();
	
	esp_lcd_panel_io_handle_t	io_handle;
	esp_lcd_panel_io_i2c_config_t io_config = install_panel_io(&io_handle);

	esp_lcd_panel_handle_t	panel_handle;
	esp_lcd_panel_dev_config_t	panel_config = install_ssd1306_panel_driver(io_handle, &panel_handle);

	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
	ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif
	//initial lvgl
	const lvgl_port_cfg_t	lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
	lvgl_port_init(&lvgl_cfg);

	const lvgl_port_display_cfg_t	disp_cfg = {
		.io_handle = io_handle,
		.panel_handle = panel_handle,
		.buffer_size = LCD_H_RES * LCD_V_RES,
		.double_buffer = true,
		.hres = LCD_H_RES,
		.vres = LCD_V_RES,
		.monochrome = true,
		.rotation = {
			.swap_xy = false,
			.mirror_x = false,
			.mirror_y = false,
		}
	};
	
	disp = lvgl_port_add_disp(&disp_cfg);

	//register done callback for io
	const esp_lcd_panel_io_callbacks_t	cbs = {
		.on_color_trans_done = notify_lvgl_flush_ready,
	};
	esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

	//rotation
	lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);
	
	//top label
	labels[0] = lv_label_create(lv_disp_get_scr_act(disp));	
	lv_label_set_long_mode(labels[0], LV_LABEL_LONG_WRAP);//long mode
	lv_obj_set_width(labels[0], disp->driver->hor_res);
	lv_obj_align(labels[0], LV_ALIGN_TOP_MID, 0, 0);
	//mid label
	labels[1] = lv_label_create(lv_disp_get_scr_act(disp));	
	lv_label_set_long_mode(labels[1], LV_LABEL_LONG_WRAP);//long mode
	lv_obj_set_width(labels[1], disp->driver->hor_res);
	lv_obj_align(labels[1], LV_ALIGN_CENTER, 0, 0);
	//low label
	labels[2] = lv_label_create(lv_disp_get_scr_act(disp));	
	lv_label_set_long_mode(labels[2], LV_LABEL_LONG_WRAP);//long mode
	lv_obj_set_width(labels[2], disp->driver->hor_res);
	lv_obj_align(labels[2], LV_ALIGN_BOTTOM_MID, 0, 0);
	//clean display
	for (int i = 0; i < 3; i++)
	{
		lv_label_set_text(labels[i], "");
	}
	//init queue
	initqueue(&g_text_queue);
}

static void	free_disp(void)
{
	delqueue(&g_text_queue, heap_caps_free);
	for (int i = 0; i < 3; i++)
	{
		lv_label_set_text(labels[i], "");
	}
}

void	print_text_lcd(char *format, ...)
{
	static int	idx = 0;
	lv_obj_t	*label;
	char	*text_val;
	
	va_list	argp;
	char	tmp[128];
	size_t	len;

	va_start(argp, format);
	//clean mem
	memset(tmp, 0, 128);
	//make va string
	vsprintf(tmp, format, argp);
	//get len
	len = strlen(tmp);
	
	printf("out: %s\n", tmp);

	va_end(argp);

	if (len > 0)
	{
		text_val = (char *)heap_caps_malloc(sizeof(char) * len + 1, MALLOC_CAP_8BIT);
		memset(text_val, 0, len + 1);
		memcpy(text_val, tmp, len);
		enqueue(&g_text_queue, (void *)text_val);

		if (idx > 2)
		{
			dequeue(&g_text_queue, heap_caps_free);
			
			for (int i = 0; i < 2; i++)
			{
				lv_label_set_text(labels[i], (char *)get_item(&g_text_queue, i));
			}
			idx = 2;
		}
		label = labels[idx++];

		lv_label_set_text(label, text_val);
	}
	else
	{
		free_disp();
		idx = 0;
	}
}
