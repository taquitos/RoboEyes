#ifndef LCD_ST7789_H
#define LCD_ST7789_H

#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

namespace lcd {

struct St7789Config {
  spi_host_device_t spi_host = SPI2_HOST;
  int spi_mode = 0;
  uint32_t pclk_hz = 40 * 1000 * 1000;
  size_t trans_queue_depth = 1;

  gpio_num_t sclk_gpio = GPIO_NUM_NC;
  gpio_num_t mosi_gpio = GPIO_NUM_NC;
  gpio_num_t cs_gpio = GPIO_NUM_NC;
  gpio_num_t dc_gpio = GPIO_NUM_NC;
  gpio_num_t rst_gpio = GPIO_NUM_NC;

  gpio_num_t backlight_gpio = GPIO_NUM_NC;
  bool backlight_active_high = true;

  int width = 240;
  int height = 280;

  // Some ST7789 variants use a 240x320 address space; smaller panels often need gaps.
  int gap_x = 0;
  int gap_y = 0;

  bool swap_xy = false;
  bool mirror_x = false;
  bool mirror_y = false;
  bool invert_color = true;
};

St7789Config supermini_c3_st7789_config_240x240();
St7789Config tinys3_st7789v3_config_240x280();

class St7789Display {
public:
  St7789Display() = default;
  ~St7789Display();

  esp_err_t init(const St7789Config &cfg);

  // Draws a full-frame RGB565 buffer of size `width` x `height`.
  esp_err_t draw_full_rgb565(const uint16_t *rgb565, int width, int height);

  // Wait for any queued color transfers to finish. This makes it safe to reuse DMA buffers.
  esp_err_t flush();

  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }

private:
  esp_lcd_panel_handle_t panel_ = nullptr;
  esp_lcd_panel_io_handle_t io_ = nullptr;
  spi_host_device_t spi_host_ = SPI2_HOST;
  bool spi_bus_initialized_ = false;

  gpio_num_t backlight_gpio_ = GPIO_NUM_NC;
  bool backlight_active_high_ = true;

  int width_ = 0;
  int height_ = 0;
};

} // namespace lcd

#endif
