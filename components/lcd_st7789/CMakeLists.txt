idf_component_register(
  SRCS
    "src/lcd_st7789.cpp"
  INCLUDE_DIRS "include"
  REQUIRES driver esp_driver_gpio esp_driver_ledc esp_lcd
)
