cmake_minimum_required(VERSION 3.16)

set(ROBOEYES_COMPONENTS eyes eyes_emotion_cycle eyes_lcd_sink lcd_st7789 pixel_convert)
include("${CMAKE_CURRENT_LIST_DIR}/../../roboeyes.cmake")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(roboeyes_st7789_supermini_c3)

