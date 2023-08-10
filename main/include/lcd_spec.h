#ifndef LCD_SPEC_H
#define LCD_SPEC_H

//////////////////////LCD SPEC/////////////////////////
#define LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define PIN_NUM_SDA	4
#define PIN_NUM_SCL	15
#define PIN_NUM_RST	16
#define I2C_HW_ADDR	0x3C

#if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
# define LCD_H_RES	128
# define LCD_V_RES	64
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
# define LCD_H_RES	64
# define LCD_V_RES	128
#endif

#define LCD_CMD_BITS	8
#define LCD_PARAM_BITS	8

#endif
