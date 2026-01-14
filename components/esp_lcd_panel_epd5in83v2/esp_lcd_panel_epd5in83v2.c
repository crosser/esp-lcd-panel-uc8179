/*
 * SPDX-FileCopyrightText: 2026 Eugene Crosser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"

#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_commands.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_check.h>
#include "esp_lcd_panel_epd5in83v2.h"

static const char *TAG = "lcd_panel_epd5in83v2";

typedef struct {
	esp_lcd_panel_t base;
	esp_lcd_panel_io_handle_t io;
	int reset_gpio_num;
	bool reset_level;
} epd5in83v2_panel_t;

static esp_err_t panel_epd5in83v2_del(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_reset(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_init(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_draw_bitmap(esp_lcd_panel_t * panel,
						int x_start, int y_start,
						int x_end, int y_end,
						const void *color_data)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_invert_color(esp_lcd_panel_t * panel,
						bool invert_color_data)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_mirror(esp_lcd_panel_t * panel,
						bool mirror_x, bool mirror_y)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_swap_xy(esp_lcd_panel_t * panel, bool swap_axes)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_set_gap(esp_lcd_panel_t * panel, int x_gap,
				       int y_gap)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_disp_on_off(esp_lcd_panel_t * panel, bool off)
{
	return ESP_OK;
}

static esp_err_t panel_epd5in83v2_sleep(esp_lcd_panel_t * panel, bool sleep)
{
	return ESP_OK;
}


static const esp_lcd_panel_t epd5in83v2_base = {
	.del = panel_epd5in83v2_del,
	.reset = panel_epd5in83v2_reset,
	.init = panel_epd5in83v2_init,
	.draw_bitmap = panel_epd5in83v2_draw_bitmap,
	.invert_color = panel_epd5in83v2_invert_color,
	.set_gap = panel_epd5in83v2_set_gap,
	.mirror = panel_epd5in83v2_mirror,
	.swap_xy = panel_epd5in83v2_swap_xy,
	.disp_on_off = panel_epd5in83v2_disp_on_off,
	.disp_sleep = panel_epd5in83v2_sleep,
};

esp_err_t esp_lcd_new_panel_epd5in83v2(const esp_lcd_panel_io_handle_t io,
					const esp_lcd_panel_dev_config_t *
					panel_dev_config,
					esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
	esp_err_t ret = ESP_OK;
	epd5in83v2_panel_t *epd5in83v2 = NULL;
	ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel,
		ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
	epd5in83v2 = calloc(1, sizeof(epd5in83v2_panel_t));
	ESP_GOTO_ON_FALSE(epd5in83v2,
		ESP_ERR_NO_MEM, err, TAG, "no mem for epd5in83v2 panel");
	if (panel_dev_config->reset_gpio_num >= 0) {
		ESP_GOTO_ON_ERROR(
			gpio_set_direction(
				panel_dev_config->reset_gpio_num,
				GPIO_MODE_OUTPUT),
			err, TAG, "configure GPIO for RST line failed");
	}
	epd5in83v2->io = io;
	epd5in83v2->reset_gpio_num = panel_dev_config->reset_gpio_num;
	epd5in83v2->reset_level = panel_dev_config->flags.reset_active_high;
	epd5in83v2->base = epd5in83v2_base;
	*ret_panel = &(epd5in83v2->base);
	ESP_LOGD(TAG, "new epd5in83v2 panel @%p", epd5in83v2);
	return ESP_OK;
err:
	if (epd5in83v2) {
		if (panel_dev_config->reset_gpio_num >= 0) {
			gpio_reset_pin(panel_dev_config->reset_gpio_num);
		}
		free(epd5in83v2);
	}
	return ret;
}
