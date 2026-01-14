/*
 * SPDX-FileCopyrightText: 2026 Eugene Crosser
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_lcd_panel_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of additional configuration needed by e-paper panel
 *        Please set the object of this struct to
 *        esp_lcd_panel_dev_config_t->vendor_config
 */
typedef struct {
	int busy_gpio_num;	/*!< GPIO num of the BUSY pin */
	bool non_copy_mode;	/*!< If the bitmap would be copied or not.
				  Image rotation and mirror is limited. */
} esp_lcd_epd5in83v2_config_t;

/**
 * @brief Create LCD panel for waveshare e-paper 5.83in V2 display
 *
 * @param[in] io LCD panel IO handle
 * @param[in] panel_dev_config general panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 *          - ESP_ERR_INVALID_ARG   if parameter is invalid
 *          - ESP_ERR_NO_MEM        if out of memory
 *          - ESP_OK                on success
 */
esp_err_t esp_lcd_new_panel_epd5in83v2(const esp_lcd_panel_io_handle_t io,
					const esp_lcd_panel_dev_config_t *
					panel_dev_config,
					esp_lcd_panel_handle_t * ret_panel);
#ifdef __cplusplus
}
#endif
