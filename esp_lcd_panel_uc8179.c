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

/* UC8179 SPI command codes */
#define CMD_PSR  0x00
#define CMD_PWR  0x01
#define CMD_POF  0x02
#define CMD_PON  0x04
#define CMD_DSLP 0x07
#define CMD_DTM1 0x10
#define CMD_DRF  0x12
#define CMD_DTM2 0x13
#define CMD_DSPI 0x15
#define CMD_CDI  0x50
#define CMD_TCON 0x60
#define CMD_TRES 0x61
#define CMD_FLG  0x71
#define CMD_PTL  0x90
#define CMD_PTIN 0x91
#define CMD_PTOUT 0x92

typedef struct {
	esp_lcd_panel_t base;
	esp_lcd_panel_io_handle_t io;
	int reset_gpio_num;
	bool reset_active_level;
	int busy_gpio_num;
	bool busy_gpio_lvl;
	int led_gpio_num;
	int width;
	int height;
	epd_on_color_trans_done_cb_t user_callback;
	void *user_ctx;
	uint8_t *_zeroes;
} uc8179_panel_t;

static esp_err_t uc8179_wait_busy(esp_lcd_panel_t *panel)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t, base);
#if 0
	esp_lcd_panel_io_handle_t io = uc8179->io;
	uint8_t flg;
#endif
	TickType_t start, end;
	int lvl;

	ESP_LOGD(TAG, "start busy_wait");
	start = xTaskGetTickCount();
	while ((lvl = gpio_get_level(uc8179->busy_gpio_num)) ==
			uc8179->busy_gpio_lvl) {
		ESP_LOGD(TAG, "Busy level is %d", lvl);
#if 0
		ESP_LOGD(TAG, "About to sent CMD_FLG");
		ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(io, CMD_FLG,
				&flg, 1),
		TAG, "CMD_PSR err");
		ESP_LOGD(TAG, "FLG %02x", flg);
#endif
		vTaskDelay(pdMS_TO_TICKS(15));
	}
	end = xTaskGetTickCount();
	ESP_LOGD(TAG, "was busy %d ticks", (int)(end - start));
	return ESP_OK;
}


static void uc8179_gpio_isr_handler(void *user_ctx)
{
	uc8179_panel_t *uc8179 = user_ctx;
	if (uc8179->led_gpio_num >= 0) {
		ESP_ERROR_CHECK(gpio_set_level(uc8179->led_gpio_num, 0));
	}
	gpio_intr_disable(uc8179->busy_gpio_num);
	if (uc8179->user_callback) {
		(uc8179->user_callback)(uc8179->user_ctx);
	}
}

static bool uc8179_io_callback(esp_lcd_panel_io_handle_t panel_io,
		esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	uc8179_panel_t *uc8179 = user_ctx;
	if (uc8179->led_gpio_num >= 0) {
		ESP_ERROR_CHECK(gpio_set_level(uc8179->led_gpio_num, 1));
	}
	gpio_intr_enable(uc8179->busy_gpio_num);
	return false;
}

esp_err_t epd_register_event_callbacks(esp_lcd_panel_t *panel,
               epd_io_callbacks_t *cbs, void *user_ctx)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t, base);
	uc8179->user_callback = cbs->on_color_trans_done;
	uc8179->user_ctx = user_ctx;
	return ESP_OK;
}

static esp_err_t panel_uc8179_del(esp_lcd_panel_t * panel)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);
	if (uc8179->reset_gpio_num >= 0) {
		gpio_reset_pin(uc8179->reset_gpio_num);
	}
	if (uc8179->busy_gpio_num >= 0) {
		gpio_reset_pin(uc8179->busy_gpio_num);
	}
	if (uc8179->led_gpio_num >= 0) {
		gpio_reset_pin(uc8179->led_gpio_num);
	}
	if (uc8179->_zeroes) {
		free(uc8179->_zeroes);
	}
	ESP_LOGD(TAG, "del uc8179 panel @%p", uc8179);
	free(uc8179);
	return ESP_OK;
}

static esp_err_t panel_uc8179_reset(esp_lcd_panel_t * panel)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);

	if (uc8179->reset_gpio_num >= 0) {
		int delays[3] = {200, 5, 200};
		int lvl = uc8179->reset_active_level;

		ESP_RETURN_ON_ERROR(gpio_set_direction(uc8179->reset_gpio_num,
				GPIO_MODE_OUTPUT),
			TAG, "configure GPIO for RST line failed");
		for (int i = 0; i < 3; i++) {
			ESP_RETURN_ON_ERROR(gpio_set_level(
					uc8179->reset_gpio_num, lvl),
				TAG, "gpio_set_level active error");
			ESP_LOGD(TAG,"reset %d, delay %d", lvl, delays[i]);
			vTaskDelay(pdMS_TO_TICKS(delays[i]));
			lvl = !lvl;
		}
	}
	return ESP_OK;
}

static esp_err_t panel_uc8179_init(esp_lcd_panel_t * panel)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);
	esp_lcd_panel_io_handle_t io = uc8179->io;

	if (uc8179->led_gpio_num >= 0) {
		ESP_RETURN_ON_ERROR(gpio_set_direction(uc8179->led_gpio_num,
				GPIO_MODE_OUTPUT),
			TAG, "led pin set direction error");
		ESP_RETURN_ON_ERROR(gpio_set_level(uc8179->led_gpio_num, 0),
			TAG, "led pin set level error");
	}
	if (uc8179->busy_gpio_num >= 0) {
		ESP_RETURN_ON_ERROR(gpio_config(&(gpio_config_t){
				.mode = GPIO_MODE_INPUT,
				.pull_down_en = 1,
				.pin_bit_mask = 1ULL <<
					uc8179->busy_gpio_num,
				.intr_type = uc8179->busy_gpio_lvl ?
					GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE,
			}), TAG, "configure GPIO for BUSY line err");
		ESP_RETURN_ON_ERROR(gpio_isr_handler_add(
				uc8179->busy_gpio_num,
				uc8179_gpio_isr_handler, uc8179
			), TAG, "configure GPIO for BUSY line err");
		gpio_intr_disable(uc8179->busy_gpio_num);
	}
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_register_event_callbacks(
			io,
			&(esp_lcd_panel_io_callbacks_t) {
				.on_color_trans_done = uc8179_io_callback
			},
			uc8179),
		TAG, "could not register spi io callbacks");
	ESP_LOGD(TAG, "About to sent CMD_PWR");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PWR,
			(uint8_t[]) { 0x07, 0x07, 0x3f, 0x3f }, 4),
		TAG, "CMD_PWR err");
	ESP_LOGD(TAG, "About to sent CMD_PON");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PON, NULL, 0),
		TAG, "CMD_PON err");
	vTaskDelay(pdMS_TO_TICKS(100));
	uc8179_wait_busy(panel);
	ESP_LOGD(TAG, "About to sent CMD_PSR");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PSR,
			(uint8_t[]) { 0x1f }, 1),
		TAG, "CMD_PSR err");
	ESP_LOGD(TAG, "About to sent CMD_TRES");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_TRES,
			(uint8_t[]) {
				uc8179->width >> 8,
				uc8179->width & 0xff,
				uc8179->height >> 8,
				uc8179->height & 0xff }, 4),
		TAG, "CMD_TRES err");
	ESP_LOGD(TAG, "About to sent CMD_DSPI");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_DSPI,
			(uint8_t[]) { 0 }, 1),
		TAG, "CMD_DSPI err");
	ESP_LOGD(TAG, "About to sent CMD_CDI");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_CDI,
			(uint8_t[]) { 0x10, 0x07 }, 2),
		TAG, "CMD_CDI err");
	ESP_LOGD(TAG, "About to sent CMD_TCON");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_TCON,
			(uint8_t[]) { 0x22 }, 1),
		TAG, "CMD_TCON err");
	return ESP_OK;
}

static esp_err_t panel_uc8179_draw_bitmap(esp_lcd_panel_t * panel,
						int x_start, int y_start,
						int x_end, int y_end,
						const void *color_data)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);
	esp_lcd_panel_io_handle_t io = uc8179->io;

	ESP_RETURN_ON_FALSE(x_start >= 0 && y_start >= 0
			&& x_end <= uc8179->width
			&& y_end <= uc8179->height
			&& x_start < x_end && y_start < y_end,
		ESP_ERR_INVALID_ARG, TAG,
		"Coordinates ourside the panel dimensions");
	bool fullscreen = x_start == 0 && y_start == 0
			&& x_end == uc8179->width && y_end == uc8179->height;
	// Could not make the thing work. The image is sheared. Disallow.
	ESP_RETURN_ON_FALSE(fullscreen, ESP_ERR_INVALID_ARG, TAG,
			"Partial update does not work, unsupported");
	if (!fullscreen) {
		ESP_LOGD(TAG, "About to set partial mode");
		ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PTIN,
				NULL, 0),
			TAG, "CMD_PTIN err");
		ESP_LOGD(TAG, "Abount to set partial window");
		ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PTL,
				(uint8_t[]) {
					x_start >> 8,
					x_start & 0xff,
					x_end >> 8,
					x_end & 0xff,
					y_start >> 8,
					y_start & 0xff,
					y_end >> 8,
					y_end & 0xff,
					0x01,
				}, 9),
			TAG, "CMD_PTL err");
	}
	if (fullscreen) {
		ESP_LOGD(TAG, "About to sent data with CMD_DTM1");
		ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, CMD_DTM1,
				uc8179->_zeroes,
				(x_end - x_start) * (y_end - y_start) / 8),
			TAG, "CMD_DTM1 err");
	}
	ESP_LOGD(TAG, "About to sent data with CMD_DTM2");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, CMD_DTM2,
			color_data, (x_end - x_start) * (y_end - y_start) / 8),
		TAG, "CMD_DTM2 err");
	ESP_LOGD(TAG, "About to sent CMD_DRF");
	// put DRF in the queue of async transactions, use "color" command
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, CMD_DRF, NULL, 0),
		TAG, "CMD_DRF err");
	if (!fullscreen) {
		ESP_LOGD(TAG, "About to reset to full mode");
		ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, CMD_PTOUT,
				NULL, 0),
			TAG, "CMD_PTOUT err");
	}

	return ESP_OK;
}

static esp_err_t panel_uc8179_disp_on_off(esp_lcd_panel_t * panel, bool on)
{
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);
	esp_lcd_panel_io_handle_t io = uc8179->io;
	ESP_LOGD(TAG, "About to sent %s", on ? "CMD_PON" : "CMD_POF");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(
			io, on ? CMD_PON : CMD_POF, NULL, 0),
		TAG, "CMD_PON err");
	vTaskDelay(pdMS_TO_TICKS(100));
	uc8179_wait_busy(panel);

	return ESP_OK;
}

static esp_err_t panel_uc8179_sleep(esp_lcd_panel_t * panel, bool sleep)
{
	ESP_RETURN_ON_FALSE(sleep,
		ESP_ERR_INVALID_ARG, TAG, "Wakeup is not supported");
	uc8179_panel_t *uc8179 = __containerof(panel, uc8179_panel_t,  base);
	esp_lcd_panel_io_handle_t io = uc8179->io;
	ESP_LOGD(TAG, "About to sent CMD_POF");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param( io, CMD_POF, NULL, 0),
		TAG, "CMD_PON err");
	vTaskDelay(pdMS_TO_TICKS(100));
	uc8179_wait_busy(panel);
	ESP_LOGD(TAG, "About to sent CMD_DSLP");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param( io, CMD_DSLP,
			(uint8_t[]) { 0xa5 }, 1),
		TAG, "CMD_DSLP err");
	return ESP_OK;
}

static const esp_lcd_panel_t uc8179_base = {
	.reset = panel_uc8179_reset,
	.init = panel_uc8179_init,
	.del = panel_uc8179_del,
	.draw_bitmap = panel_uc8179_draw_bitmap,
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
	ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel,
		ESP_ERR_INVALID_ARG, TAG, "1 or more args is NULL");
	esp_lcd_uc8179_config_t *uc8179_config =
		panel_dev_config->vendor_config;
	esp_err_t ret = ESP_OK;
	uc8179_panel_t *uc8179 = NULL;
	ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel,
		ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
	uc8179 = calloc(1, sizeof(uc8179_panel_t));
	ESP_GOTO_ON_FALSE(uc8179,
		ESP_ERR_NO_MEM, err, TAG, "no mem for uc8179 panel");
	uc8179->led_gpio_num = uc8179_config->led_gpio_num;
	uc8179->reset_gpio_num = panel_dev_config->reset_gpio_num;
	uc8179->reset_active_level = panel_dev_config->flags.reset_active_high;
	uc8179->busy_gpio_num = uc8179_config->busy_gpio_num;
	uc8179->busy_gpio_lvl = uc8179_config->busy_gpio_lvl;
	uc8179->width = uc8179_config->width;
	uc8179->height = uc8179_config->height;
	uc8179->io = io;
	uc8179->base = uc8179_base;
	uc8179->_zeroes = heap_caps_malloc(
		uc8179->width * uc8179->height / 8, MALLOC_CAP_DMA);
	ESP_GOTO_ON_FALSE(uc8179->_zeroes, ESP_ERR_NO_MEM, err, TAG,
			"uc8179 allocating zero buffer err");
	memset(uc8179->_zeroes, 0, uc8179->width * uc8179->height / 8);

	*ret_panel = &(uc8179->base);
	ESP_LOGD(TAG, "new uc8179 panel @%p", uc8179);
	return ESP_OK;
err:
	if (uc8179) {
		free(uc8179);
	}
	return ret;
}
