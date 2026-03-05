#include <esp_log.h>
#include <lvgl.h>
#include <misc/lv_style.h>
#include "display.h"

static const char *TAG = "LV display";

static LV_STYLE_CONST_INIT(screen_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0, 0, 0)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
	}));

void init_display(lv_display_t *disp)
{
	ESP_LOGI(TAG, "init_display");
	lv_obj_t *scr = lv_display_get_screen_active(disp);
	lv_obj_add_style(scr, &screen_style, LV_PART_MAIN);
	lv_obj_clean(scr);
}

void stop_display(lv_display_t *disp)
{
	ESP_LOGI(TAG, "stop_display");
	lv_obj_t *scr = lv_display_get_screen_active(disp);
	lv_obj_clean(scr);
}
