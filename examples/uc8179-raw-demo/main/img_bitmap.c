#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include "img_bitmap.h"
#include "img.h"

#define TAG "img_bitmap"

void make_bitmap(size_t w, size_t h, uint8_t *buffer)
{
	ESP_LOGI(TAG, "source %zdx%zd, dest %zdx%zd",
			SOURCE_WIDTH, SOURCE_HEIGHT, w, h);
	ESP_ERROR_CHECK(w >= SOURCE_WIDTH && h >= SOURCE_HEIGHT
			? ESP_OK : ESP_FAIL);
	size_t w_off = (w - SOURCE_WIDTH) / 2;
	size_t h_off = (h - SOURCE_HEIGHT) / 2;
	ESP_LOGI(TAG, "horizontal offset %zd (%d bytes), vertical offset %zd",
			w_off, w_off / 8, h_off);
	memset(buffer, 0, w * h / 8);
	for (size_t i = h_off; i < h; i++)
		memcpy(buffer + (w_off + i * w) / 8,
			MagickImage + i * SOURCE_WIDTH / 8,
			SOURCE_WIDTH / 8);
}
