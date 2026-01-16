#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_uc8179.h>

#define TAG "panel_uc8179"

#if defined(CONFIG_HWE_DISPLAY_SPI1_HOST)
# define SPIx_HOST SPI1_HOST
#elif defined(CONFIG_HWE_DISPLAY_SPI2_HOST)
# define SPIx_HOST SPI2_HOST
#else
# error "SPI host 1 or 2 must be selected"
#endif

#define BLINK_TIME (2 * 1000000)

static bool led_on = false;

static void blink_task(void *arg)
{
	led_on = !led_on;
	ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_2, led_on));
}

void app_main(void)
{
	/* Just to make sure the board works, set up 2sec on 2 off blink */
	ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT));
	esp_timer_handle_t blink_timer;
	ESP_ERROR_CHECK(esp_timer_create(
		&(esp_timer_create_args_t) {
			.callback = blink_task,
			.name = "blink timer",
		},
		&blink_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, BLINK_TIME));
	ESP_LOGI(TAG, "Blinker set up.");

	ESP_LOGI(TAG, "Initializing SPI bus");
	ESP_ERROR_CHECK(spi_bus_initialize(SPIx_HOST,
		&(spi_bus_config_t) {
			.mosi_io_num = CONFIG_HWE_DISPLAY_SPI_MOSI,
			.miso_io_num = -1,
			.sclk_io_num = CONFIG_HWE_DISPLAY_SPI_SCK,
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
			.lcd_cmd_bits = 0,
			.lcd_param_bits = 0,
			.trans_queue_depth = 17,
		},
		&io_handle
	));
	// NOTE: Please call gpio_install_isr_service() manually
	// before esp_lcd_new_panel_uc8179() because gpio_isr_handler_add()
	// is called in esp_lcd_new_panel_uc8179()
	gpio_install_isr_service(0);
	esp_lcd_panel_handle_t panel_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_uc8179(io_handle,
		&(esp_lcd_panel_dev_config_t) {
			.reset_gpio_num = GPIO_NUM_26,
			.flags.reset_active_high =
				CONFIG_HWE_DISPLAY_RST_ACTIVE_LEVEL,
			.vendor_config =
				&(esp_lcd_uc8179_config_t) {
					.busy_gpio_num =
						CONFIG_HWE_DISPLAY_BUSY,
					.non_copy_mode = true,
				},
		},
		&panel_handle));
}
