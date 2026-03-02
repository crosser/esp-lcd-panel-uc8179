#include <stdint.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_uc8179.h>

#include "sdkconfig.h"
#include "img_bitmap.h"

#define TAG "panel_uc8179"

#if defined(CONFIG_HWE_DISPLAY_SPI1_HOST)
# define SPIx_HOST SPI1_HOST
#elif defined(CONFIG_HWE_DISPLAY_SPI2_HOST)
# define SPIx_HOST SPI2_HOST
#else
# error "SPI host 1 or 2 must be selected"
#endif

#define BITMAP_SIZE (CONFIG_HWE_DISPLAY_WIDTH * CONFIG_HWE_DISPLAY_HEIGHT / 8)

static bool give_semaphore_in_isr(void *user_ctx)
{
	SemaphoreHandle_t *epd_ready_ptr = user_ctx;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(*epd_ready_ptr, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
		return true;
	}
	return false;
}

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPI bus");
	ESP_ERROR_CHECK(spi_bus_initialize(SPIx_HOST,
		&(spi_bus_config_t) {
			.mosi_io_num = CONFIG_HWE_DISPLAY_SPI_MOSI,
			.miso_io_num = CONFIG_HWE_DISPLAY_SPI_MISO,
			.sclk_io_num = CONFIG_HWE_DISPLAY_SPI_SCK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
			.flags = SPICOMMON_BUSFLAG_MASTER
				| SPICOMMON_BUSFLAG_GPIO_PINS,
		},
		SPI_DMA_CH_AUTO
	));
	ESP_LOGI(TAG, "Attach panel IO handle to SPI");
	esp_lcd_panel_io_handle_t io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
		(esp_lcd_spi_bus_handle_t)SPIx_HOST,
		&(esp_lcd_panel_io_spi_config_t) {
			.cs_gpio_num = CONFIG_HWE_DISPLAY_SPI_CS,
			.dc_gpio_num = CONFIG_HWE_DISPLAY_SPI_DC,
			.spi_mode = 0,
			.pclk_hz = CONFIG_HWE_DISPLAY_SPI_FREQUENCY,
			.lcd_cmd_bits = 8,
			.lcd_param_bits = 8,
			.trans_queue_depth = 17,
		},
		&io_handle
	));
	// NOTE: Please call gpio_install_isr_service() manually
	// before esp_lcd_new_panel_uc8179() because gpio_isr_handler_add()
	// is called in esp_lcd_new_panel_uc8179()
	ESP_LOGI(TAG, "Install ISR service for GPIO");
	gpio_install_isr_service(0);
	ESP_LOGI(TAG, "Initialize uc8179 panel driver");
	esp_lcd_panel_handle_t panel_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_uc8179(io_handle,
		&(esp_lcd_panel_dev_config_t) {
			.reset_gpio_num = CONFIG_HWE_DISPLAY_RST,
			.flags.reset_active_high =
				CONFIG_HWE_DISPLAY_RST_ACTIVE_LEVEL,
			.vendor_config =
				&(esp_lcd_uc8179_config_t) {
					.busy_gpio_num =
						CONFIG_HWE_DISPLAY_BUSY,
					.width = CONFIG_HWE_DISPLAY_WIDTH,
					.height = CONFIG_HWE_DISPLAY_HEIGHT,
				},
		},
		&panel_handle));
	ESP_LOGI(TAG, "Resetting e-Paper display...");
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	vTaskDelay(100 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Initializing e-Paper display...");
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
	vTaskDelay(100 / portTICK_PERIOD_MS);
	// ESP_ERROR_CHECK(epaper_panel_set_custom_lut(panel_handle, ...);
	// vTaskDelay(100 / portTICK_PERIOD_MS);

	SemaphoreHandle_t epd_ready = xSemaphoreCreateBinary();
	// Semaphore is created initially "taken"
	ESP_ERROR_CHECK(epd_register_event_callbacks(
		panel_handle,
		&(epd_io_callbacks_t) {
			.on_color_trans_done = give_semaphore_in_isr,
		},
		&epd_ready));

	ESP_LOGI(TAG, "Clear screen");
	uint8_t *empty_bitmap = heap_caps_malloc(BITMAP_SIZE, MALLOC_CAP_DMA);
	memset(empty_bitmap, 0, BITMAP_SIZE);
	esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, CONFIG_HWE_DISPLAY_WIDTH,
			CONFIG_HWE_DISPLAY_HEIGHT, empty_bitmap);
	ESP_LOGI(TAG, "Waiting for completion");
	xSemaphoreTake(epd_ready, portMAX_DELAY);
	ESP_LOGI(TAG, "Completion");

	vTaskDelay(pdMS_TO_TICKS(5000));
	ESP_LOGI(TAG, "Drawing bitmap...");
	ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
			CONFIG_HWE_DISPLAY_WIDTH, CONFIG_HWE_DISPLAY_HEIGHT,
			BITMAP));
	ESP_LOGI(TAG, "Waiting for completion");
	xSemaphoreTake(epd_ready, portMAX_DELAY);
	ESP_LOGI(TAG, "Completion");
	ESP_LOGI(TAG, "Put panel to sleep...");
	esp_lcd_panel_disp_sleep(panel_handle, true);
	ESP_LOGI(TAG, "Go to deep sleep mode...");
	esp_deep_sleep_start();
}
