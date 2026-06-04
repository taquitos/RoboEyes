idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "."
  REQUIRES eyes eyes_emotion_cycle eyes_lcd_sink lcd_st7789 pixel_convert
)

