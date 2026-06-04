#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "eyes/eyes.h"
#include "eyes_emotion_cycle.h"
#include "eyes_lcd_sink.h"
#include "lcd_st7789.h"

namespace {

constexpr char TAG[] = "roboeyes_st7789_c3";

constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 240;
constexpr int LCD_GAP_X = 0;
constexpr int LCD_GAP_Y = 0;

lcd::St7789Config make_lcd_config() {
  lcd::St7789Config cfg;
  cfg.spi_host = SPI2_HOST;
  cfg.sclk_gpio = GPIO_NUM_6;
  cfg.mosi_gpio = GPIO_NUM_7;
  cfg.cs_gpio = GPIO_NUM_10;
  cfg.dc_gpio = GPIO_NUM_4;
  cfg.rst_gpio = GPIO_NUM_3;
  cfg.backlight_gpio = GPIO_NUM_5;
  cfg.backlight_active_high = true;
  cfg.width = LCD_WIDTH;
  cfg.height = LCD_HEIGHT;
  cfg.gap_x = LCD_GAP_X;
  cfg.gap_y = LCD_GAP_Y;
  cfg.invert_color = true;
  return cfg;
}

} // namespace

extern "C" void app_main() {
  ESP_LOGI(TAG, "RoboEyes ST7789 Super Mini boot");

  const lcd::St7789Config lcd_cfg = make_lcd_config();
  lcd::St7789Display display;
  ESP_ERROR_CHECK(display.init(lcd_cfg));

  eyes_lcd::EyesLcdSinkConfig sink_cfg;
  sink_cfg.display = &display;
  eyes_lcd::EyesLcdSink sink(sink_cfg);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions options;
  options.size = {lcd_cfg.width, lcd_cfg.height};
  options.fps = 30;
  options.realtime = true;

  if (!eyes_cycle::run_forever(player, options)) {
    ESP_LOGE(TAG, "Emotion cycle stopped");
  }
}

