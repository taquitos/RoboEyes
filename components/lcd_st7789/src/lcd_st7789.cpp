#include "lcd_st7789.h"

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"

namespace lcd {

namespace {

static const char *TAG = "lcd_st7789";
constexpr ledc_mode_t kBacklightLedcMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kBacklightLedcTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kBacklightLedcChannel = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t kBacklightLedcDutyResolution = LEDC_TIMER_10_BIT;
constexpr uint32_t kBacklightLedcFrequencyHz = 5000;
constexpr uint32_t kBacklightLedcMaxDuty = (1U << 10) - 1U;
constexpr uint32_t kBacklightLedcDuty80Percent = (kBacklightLedcMaxDuty * 80U) / 100U;

constexpr gpio_num_t gpio(int value) {
  return static_cast<gpio_num_t>(value);
}

static esp_err_t configure_backlight_pwm(gpio_num_t gpio, bool active_high) {
  if (gpio == GPIO_NUM_NC) {
    return ESP_OK;
  }

  ledc_timer_config_t timer_cfg = {};
  timer_cfg.speed_mode = kBacklightLedcMode;
  timer_cfg.duty_resolution = kBacklightLedcDutyResolution;
  timer_cfg.timer_num = kBacklightLedcTimer;
  timer_cfg.freq_hz = kBacklightLedcFrequencyHz;
  timer_cfg.clk_cfg = LEDC_AUTO_CLK;
  ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "backlight ledc timer failed");

  ledc_channel_config_t channel_cfg = {};
  channel_cfg.gpio_num = gpio;
  channel_cfg.speed_mode = kBacklightLedcMode;
  channel_cfg.channel = kBacklightLedcChannel;
  channel_cfg.intr_type = LEDC_INTR_DISABLE;
  channel_cfg.timer_sel = kBacklightLedcTimer;
  channel_cfg.duty = 0;
  channel_cfg.hpoint = 0;
  channel_cfg.flags.output_invert = !active_high;
  return ledc_channel_config(&channel_cfg);
}

static esp_err_t set_backlight_pwm(uint32_t duty) {
  ESP_RETURN_ON_ERROR(ledc_set_duty(kBacklightLedcMode, kBacklightLedcChannel, duty), TAG,
                      "backlight ledc set duty failed");
  return ledc_update_duty(kBacklightLedcMode, kBacklightLedcChannel);
}

static void stop_backlight_pwm(bool active_high) {
  uint32_t idle_level = active_high ? 0U : 1U;
  (void)ledc_stop(kBacklightLedcMode, kBacklightLedcChannel, idle_level);
}

static void reset_backlight_gpio(gpio_num_t gpio) {
  if (gpio == GPIO_NUM_NC) {
    return;
  }
  gpio_reset_pin(gpio);
}

} // namespace

St7789Config supermini_c3_st7789_config_240x240() {
  St7789Config cfg;
  cfg.spi_host = SPI2_HOST;
  cfg.sclk_gpio = GPIO_NUM_6;
  cfg.mosi_gpio = GPIO_NUM_7;
  cfg.cs_gpio = GPIO_NUM_10;
  cfg.dc_gpio = GPIO_NUM_4;
  cfg.rst_gpio = GPIO_NUM_3;
  cfg.backlight_gpio = GPIO_NUM_5;
  cfg.backlight_active_high = true;
  cfg.width = 240;
  cfg.height = 240;
  cfg.gap_x = 0;
  cfg.gap_y = 0;
  cfg.invert_color = true;
  return cfg;
}

St7789Config tinys3_st7789v3_config_240x280() {
  St7789Config cfg;
  cfg.spi_host = SPI2_HOST;
  cfg.sclk_gpio = gpio(36);
  cfg.mosi_gpio = gpio(35);
  cfg.cs_gpio = gpio(34);
  cfg.dc_gpio = GPIO_NUM_7;
  cfg.rst_gpio = GPIO_NUM_6;
  cfg.backlight_gpio = GPIO_NUM_21;
  cfg.backlight_active_high = true;
  cfg.width = 240;
  cfg.height = 280;
  cfg.gap_x = 20;
  cfg.gap_y = 0;
  cfg.swap_xy = true;
  cfg.mirror_y = true;
  cfg.invert_color = true;
  return cfg;
}

St7789Display::~St7789Display() {
  // Best-effort cleanup. This typically isn't called in firmware, but helps tests/tools.
  if (backlight_gpio_ != GPIO_NUM_NC) {
    if (backlight_pwm_initialized_) {
      stop_backlight_pwm(backlight_active_high_);
      backlight_pwm_initialized_ = false;
    }
    reset_backlight_gpio(backlight_gpio_);
  }
  if (panel_) {
    (void)esp_lcd_panel_del(panel_);
    panel_ = nullptr;
  }
  if (io_) {
    (void)esp_lcd_panel_io_del(io_);
    io_ = nullptr;
  }
  if (spi_bus_initialized_) {
    (void)spi_bus_free(spi_host_);
    spi_bus_initialized_ = false;
  }
}

esp_err_t St7789Display::init(const St7789Config &cfg) {
  if (cfg.width <= 0 || cfg.height <= 0) {
    return ESP_ERR_INVALID_ARG;
  }
  if (cfg.sclk_gpio == GPIO_NUM_NC || cfg.mosi_gpio == GPIO_NUM_NC || cfg.dc_gpio == GPIO_NUM_NC) {
    return ESP_ERR_INVALID_ARG;
  }

  width_ = cfg.width;
  height_ = cfg.height;
  spi_host_ = cfg.spi_host;

  // Backlight off during init, if present.
  backlight_gpio_ = cfg.backlight_gpio;
  backlight_active_high_ = cfg.backlight_active_high;
  if (backlight_gpio_ != GPIO_NUM_NC) {
    ESP_RETURN_ON_ERROR(configure_backlight_pwm(backlight_gpio_, backlight_active_high_), TAG,
                        "backlight pwm config failed");
    backlight_pwm_initialized_ = true;
  }

  ESP_LOGI(TAG, "Init SPI bus (host=%d, sclk=%d, mosi=%d, cs=%d, dc=%d, rst=%d)", (int)cfg.spi_host,
           (int)cfg.sclk_gpio, (int)cfg.mosi_gpio, (int)cfg.cs_gpio, (int)cfg.dc_gpio,
           (int)cfg.rst_gpio);

  spi_bus_config_t bus_cfg = {};
  bus_cfg.sclk_io_num = cfg.sclk_gpio;
  bus_cfg.mosi_io_num = cfg.mosi_gpio;
  bus_cfg.miso_io_num = GPIO_NUM_NC;
  bus_cfg.quadwp_io_num = GPIO_NUM_NC;
  bus_cfg.quadhd_io_num = GPIO_NUM_NC;
  bus_cfg.max_transfer_sz = cfg.width * cfg.height * 2;
  ESP_RETURN_ON_ERROR(spi_bus_initialize(cfg.spi_host, &bus_cfg, SPI_DMA_CH_AUTO), TAG,
                      "spi_bus_initialize failed");
  spi_bus_initialized_ = true;

  esp_lcd_panel_io_spi_config_t io_cfg = {};
  io_cfg.cs_gpio_num = cfg.cs_gpio;
  io_cfg.dc_gpio_num = cfg.dc_gpio;
  io_cfg.spi_mode = cfg.spi_mode;
  io_cfg.pclk_hz = cfg.pclk_hz;
  io_cfg.trans_queue_depth = cfg.trans_queue_depth;
  io_cfg.lcd_cmd_bits = 8;
  io_cfg.lcd_param_bits = 8;
  ESP_RETURN_ON_ERROR(
      esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)cfg.spi_host, &io_cfg, &io_), TAG,
      "esp_lcd_new_panel_io_spi failed");

  esp_lcd_panel_dev_config_t panel_cfg = {};
  panel_cfg.reset_gpio_num = cfg.rst_gpio;
  panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
  panel_cfg.bits_per_pixel = 16;
  ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io_, &panel_cfg, &panel_), TAG,
                      "esp_lcd_new_panel_st7789 failed");

  ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_), TAG, "panel reset failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_), TAG, "panel init failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_, true), TAG, "panel on failed");

  ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_, cfg.invert_color), TAG, "invert failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel_, cfg.gap_x, cfg.gap_y), TAG, "set_gap failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel_, cfg.swap_xy), TAG, "swap_xy failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_, cfg.mirror_x, cfg.mirror_y), TAG, "mirror failed");

  if (backlight_pwm_initialized_) {
    ESP_RETURN_ON_ERROR(set_backlight_pwm(kBacklightLedcDuty80Percent), TAG,
                        "backlight pwm duty failed");
  }

  ESP_LOGI(TAG, "ST7789 ready (%dx%d gap=%d,%d swap_xy=%d mirror=%d,%d invert=%d backlight=%u%%)",
           cfg.width, cfg.height, cfg.gap_x, cfg.gap_y, (int)cfg.swap_xy, (int)cfg.mirror_x,
           (int)cfg.mirror_y, (int)cfg.invert_color, 80U);
  return ESP_OK;
}

esp_err_t St7789Display::draw_full_rgb565(const uint16_t *rgb565, int width, int height) {
  return draw_rgb565_area(rgb565, 0, 0, width, height);
}

esp_err_t St7789Display::draw_rgb565_area(const uint16_t *rgb565, int x, int y, int width,
                                          int height) {
  if (!panel_ || !rgb565) {
    return ESP_ERR_INVALID_STATE;
  }
  if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > width_ ||
      y + height > height_) {
    return ESP_ERR_INVALID_ARG;
  }

  return esp_lcd_panel_draw_bitmap(panel_, x, y, x + width, y + height, rgb565);
}

esp_err_t St7789Display::flush() {
  if (!io_) {
    return ESP_ERR_INVALID_STATE;
  }
  // This flushes any in-flight queued color transactions.
  return esp_lcd_panel_io_tx_param(io_, -1, nullptr, 0);
}

} // namespace lcd
