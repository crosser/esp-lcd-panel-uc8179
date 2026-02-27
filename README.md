ESP32 e-paper driver board from Waveshare uses this pins:

```
/**
 * GPIO config
**/
#define EPD_SCK_PIN  13
#define EPD_MOSI_PIN 14
#define EPD_CS_PIN   15
#define EPD_RST_PIN  26
#define EPD_DC_PIN   27
#define EPD_BUSY_PIN 25
```

We connect to Waveshare b/w 5in83 panel V2 which is presumably the same
thing as Good Display model GDEY0583T81 that uses IC UC8179.
Resolution is 648x480.
See https://www.good-display.com/product/440.html

Inspiration for the driver:

[esp-bsp driver for ssd1681](
https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_ssd1681
)

Working Arduino code ${Arduino}/libraries/esp32-waveshare-epd/src/

[github repo](https://github.com/waveshareteam/e-Paper)
