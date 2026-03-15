# esp\_lcd\_panel driver for e-paper panels based on UC8179

Developed using

- Waveshare 5in83 panel V2

Probably will work with

- Good Display model GDEY0583T81
- Waveshare 7in5 panel V2

Possibly with other e-paper panels that use UC8179 IC

# Design and use

For SPI-driven E-Paper displays, existing esp-idf framework for LCD displays
is almost, but not entirely, sufficient. Like with LCD / OLED panels, MCU
needs to transfer image pixmap (bitmap in the case of B/W e-paper) data
to the panel's memory over SPI. But unlike LCD/OLED, e-paper panels typically
need to be "refreshed" via a separate SPI transaction _after_ image data has
been transferred. After this command is sent, display spends some seconds
refreshing the picture, and reports completion by changing state of a
dedicated GPIO pin.

Thus, program flow cannot rely on the callback notification sent by the
esp-lcd framework once SPI transfer is complete, but need to keep track
of the "refresh complete" GPIO notification.

This driver uses SPI callback to issue "refresh" command. User program
_must not_ register such callback. Instead the user program may register
driver-specific callback that will be called when refresh is complete.

Also, the calling program _must_ issue `gpio_install_isr_service()` as
it is used by the driver.

See examples for supported workflow.

# Inspiration

[esp-bsp driver for ssd1681](
https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_ssd1681
)

Working Arduino code ${Arduino}/libraries/esp32-waveshare-epd/src/

[github repo](https://github.com/waveshareteam/e-Paper)
