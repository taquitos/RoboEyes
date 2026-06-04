set(ROBOEYES_FRAMEWORK_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "RoboEyes framework directory")

set(ROBOEYES_AVAILABLE_COMPONENTS
  eyes
  eyes_emotion_cycle
  eyes_log_sink
  eyes_lcd_sink
  lcd_st7789
  pixel_convert
)

if(NOT DEFINED ROBOEYES_COMPONENTS)
  set(ROBOEYES_COMPONENTS ${ROBOEYES_AVAILABLE_COMPONENTS})
endif()

set(ROBOEYES_COMPONENT_DIRS)
foreach(component IN LISTS ROBOEYES_COMPONENTS)
  if(NOT "${component}" IN_LIST ROBOEYES_AVAILABLE_COMPONENTS)
    message(FATAL_ERROR "Unknown RoboEyes component: ${component}")
  endif()
  list(APPEND ROBOEYES_COMPONENT_DIRS "${ROBOEYES_FRAMEWORK_DIR}/components/${component}")
endforeach()

list(APPEND EXTRA_COMPONENT_DIRS ${ROBOEYES_COMPONENT_DIRS})
