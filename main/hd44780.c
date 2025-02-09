#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
//#include <sys/time.h>

#include <parameter.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>

extern QueueHandle_t xQueueParameter;

void hd44780(void *pvParameters)
{
	hd44780_t lcd = {
		.write_cb = NULL,
		.font = HD44780_FONT_5X8,
		.lines = 2,
		.pins = {
			.rs = CONFIG_RS_GPIO,
			.e	= CONFIG_E_GPIO,
			.d4 = CONFIG_D4_GPIO,
			.d5 = CONFIG_D5_GPIO,
			.d6 = CONFIG_D6_GPIO,
			.d7 = CONFIG_D7_GPIO,
			.bl = HD44780_NOT_USED
		}
	};

	// Enable backlight control
	if (CONFIG_BL_GPIO != -1) {
		lcd.pins.bl = CONFIG_BL_GPIO;
		ESP_LOGW(pcTaskGetName(0), "GPIO%d is used for backlight control.", CONFIG_BL_GPIO);
	}

	// Enable 4-line display
#if CONFIG_4LINE
	lcd.lines = 4;
	ESP_LOGW(pcTaskGetName(0), "Enable 4-line display.");
#endif

	ESP_ERROR_CHECK(hd44780_init(&lcd));

	PARAMETER_t paramBuf;
	bool _lcd = true;
	bool _cursor = false;
	bool _blink = false;
	uint8_t _col = 0;
	uint8_t _line = 0;

	while(1) {
		// Wait for input
		ESP_LOGI(pcTaskGetName(0),"Waitting ....");
		xQueueReceive(xQueueParameter, &paramBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(0), "paramBuf.function=[%s]", paramBuf.function);
	
		if (strcmp(paramBuf.function, "control") == 0) {
			ESP_LOGI(pcTaskGetName(0),"control lcd=%d cursor=%d blink=%d", paramBuf.lcd, paramBuf.cursor, paramBuf.blink);
			hd44780_control(&lcd, paramBuf.lcd, paramBuf.cursor, paramBuf.blink);
		} else if (strcmp(paramBuf.function, "clear") == 0) {
			hd44780_clear(&lcd);
		} else if (strcmp(paramBuf.function, "gotoxy") == 0) {
			ESP_LOGI(pcTaskGetName(0),"gotoxy col=%d line=%d", paramBuf.col, paramBuf.line);
			hd44780_gotoxy(&lcd, paramBuf.col, paramBuf.line);
		} else if (strcmp(paramBuf.function, "putc") == 0) {
			ESP_LOGI(pcTaskGetName(0),"putc ch=[%c]", paramBuf.ch);
			hd44780_putc(&lcd, paramBuf.ch);
		} else if (strcmp(paramBuf.function, "puts") == 0) {
			ESP_LOGI(pcTaskGetName(0),"puts str=[%s]", paramBuf.str);
			hd44780_puts(&lcd, paramBuf.str);
		} else if (strcmp(paramBuf.function, "backlight") == 0) {
			ESP_LOGI(pcTaskGetName(0),"backlight backlight=%d", paramBuf.backlight);
			if (CONFIG_BL_GPIO != -1) {
				ESP_LOGW(pcTaskGetName(0),"Backlight control disabled");
			} else {
				hd44780_switch_backlight(&lcd, paramBuf.backlight);
			}
		} else if (strcmp(paramBuf.function, "col") == 0) {
			ESP_LOGI(pcTaskGetName(0),"col=%d", paramBuf.col);
			_col = paramBuf.col;
			ESP_LOGI(pcTaskGetName(0),"_col=%d _line=%d", _col, _line);
			hd44780_gotoxy(&lcd, _col, _line);
		} else if (strcmp(paramBuf.function, "line") == 0) {
			ESP_LOGI(pcTaskGetName(0),"line=%d", paramBuf.line);
			_line = paramBuf.line;
			ESP_LOGI(pcTaskGetName(0),"_col=%d _line=%d", _col, _line);
			hd44780_gotoxy(&lcd, _col, _line);
		} else if (strcmp(paramBuf.function, "cursor") == 0) {
			_cursor = paramBuf.cursor;
			ESP_LOGI(pcTaskGetName(0),"_lcd=%d _cursor=%d _blink=%d", _lcd, _cursor, _blink);
			hd44780_control(&lcd, _lcd, _cursor, _blink);
		} else if (strcmp(paramBuf.function, "blink") == 0) {
			_blink = paramBuf.blink;
			ESP_LOGI(pcTaskGetName(0),"_lcd=%d _cursor=%d _blink=%d", _lcd, _cursor, _blink);
			hd44780_control(&lcd, _lcd, _cursor, _blink);
		}
	}

	/* Never reach here */
	vTaskDelete( NULL );
}

