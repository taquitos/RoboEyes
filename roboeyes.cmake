set(ROBOEYES_FRAMEWORK_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "RoboEyes framework directory")

set(ROBOEYES_COMPONENT_DIRS
  "${ROBOEYES_FRAMEWORK_DIR}/components/eyes"
  "${ROBOEYES_FRAMEWORK_DIR}/components/eyes_emotion_cycle"
  "${ROBOEYES_FRAMEWORK_DIR}/components/eyes_lcd_sink"
  "${ROBOEYES_FRAMEWORK_DIR}/components/lcd_st7789"
  "${ROBOEYES_FRAMEWORK_DIR}/components/pixel_convert"
)

list(APPEND EXTRA_COMPONENT_DIRS ${ROBOEYES_COMPONENT_DIRS})
