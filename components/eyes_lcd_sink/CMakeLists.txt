idf_component_register(
  SRCS
    "src/eyes_lcd_sink.cpp"
  INCLUDE_DIRS "include"
  REQUIRES eyes lcd_st7789 pixel_convert freertos esp_driver_gpio esp_timer esp_rom
)
