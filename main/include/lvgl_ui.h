#ifndef LGVL_UI_H
#define LGVL_UI_H

#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
# include "esp_lcd_sh1107.h"
#else
# include "esp_lcd_panel_vendor.h"
#endif

#include "lcd_spec.h"

#define I2C_HOST 0

extern char	g_display_buf[128];

void	init_disp(void);
void	print_text_lcd(char *format, ...);

#endif
