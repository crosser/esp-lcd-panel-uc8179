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
#include "esp_lcd_panel_uc8179.h"

static const char *TAG = "lcd_panel_uc8179";

typedef struct {
	esp_lcd_panel_t base;
	esp_lcd_panel_io_handle_t io;
	int reset_gpio_num;
	bool reset_level;
} uc8179_panel_t;

static esp_err_t panel_uc8179_del(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_reset(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_init(esp_lcd_panel_t * panel)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_draw_bitmap(esp_lcd_panel_t * panel,
						int x_start, int y_start,
						int x_end, int y_end,
						const void *color_data)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_invert_color(esp_lcd_panel_t * panel,
						bool invert_color_data)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_mirror(esp_lcd_panel_t * panel,
						bool mirror_x, bool mirror_y)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_swap_xy(esp_lcd_panel_t * panel, bool swap_axes)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_set_gap(esp_lcd_panel_t * panel, int x_gap,
				       int y_gap)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_disp_on_off(esp_lcd_panel_t * panel, bool off)
{
	return ESP_OK;
}

static esp_err_t panel_uc8179_sleep(esp_lcd_panel_t * panel, bool sleep)
{
	return ESP_OK;
}


static const esp_lcd_panel_t uc8179_base = {
	.del = panel_uc8179_del,
	.reset = panel_uc8179_reset,
	.init = panel_uc8179_init,
	.draw_bitmap = panel_uc8179_draw_bitmap,
	.invert_color = panel_uc8179_invert_color,
	.set_gap = panel_uc8179_set_gap,
	.mirror = panel_uc8179_mirror,
	.swap_xy = panel_uc8179_swap_xy,
	.disp_on_off = panel_uc8179_disp_on_off,
	.disp_sleep = panel_uc8179_sleep,
};

esp_err_t esp_lcd_new_panel_uc8179(const esp_lcd_panel_io_handle_t io,
					const esp_lcd_panel_dev_config_t *
					panel_dev_config,
					esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
	esp_err_t ret = ESP_OK;
	uc8179_panel_t *uc8179 = NULL;
	ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel,
		ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
	uc8179 = calloc(1, sizeof(uc8179_panel_t));
	ESP_GOTO_ON_FALSE(uc8179,
		ESP_ERR_NO_MEM, err, TAG, "no mem for uc8179 panel");
	if (panel_dev_config->reset_gpio_num >= 0) {
		ESP_GOTO_ON_ERROR(
			gpio_set_direction(
				panel_dev_config->reset_gpio_num,
				GPIO_MODE_OUTPUT),
			err, TAG, "configure GPIO for RST line failed");
	}
	uc8179->io = io;
	uc8179->reset_gpio_num = panel_dev_config->reset_gpio_num;
	uc8179->reset_level = panel_dev_config->flags.reset_active_high;
	uc8179->base = uc8179_base;
	*ret_panel = &(uc8179->base);
	ESP_LOGD(TAG, "new uc8179 panel @%p", uc8179);
	return ESP_OK;
err:
	if (uc8179) {
		if (panel_dev_config->reset_gpio_num >= 0) {
			gpio_reset_pin(panel_dev_config->reset_gpio_num);
		}
		free(uc8179);
	}
	return ret;
}
