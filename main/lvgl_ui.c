#include "lvgl_ui.h"

char	g_display_buf[128];

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

	//clean buffer
	memset(g_display_buf, 0, 128);
}

void	print_text_lcd(char *format, ...)
{
	static char	*buf = g_display_buf;
	static lv_obj_t	*label = NULL;
	
	va_list	argp;
	char	tmp[128];
	size_t	len;


	if (label != NULL)
	{
		lv_obj_del(label);
	}

	va_start(argp, format);
	//clean mem
	memset(tmp, 0, 128);
	//make va string
	vsprintf(tmp, format, argp);
	//get len
	len = strlen(tmp);
	
	printf("out: %s\n", tmp);

	va_end(argp);

	if (len == 1 || (buf - g_display_buf) + len > 128)
	{
		//clear display
		memset(g_display_buf, 0, 128);
		buf = g_display_buf;
	}
	//cpy to buffer
	if (len > 1)
		memcpy(buf, tmp, len);

	lv_obj_t	*scr = lv_disp_get_scr_act(disp);
	label = lv_label_create(scr);

	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);//long mode
	//print global buffer
	lv_label_set_text(label, g_display_buf);

	lv_obj_set_width(label, disp->driver->hor_res);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

	if (len > 1)
		buf += len;
}
