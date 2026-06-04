cmake_minimum_required(VERSION 3.16)

set(ROBOEYES_PRESET headless)
include("${CMAKE_CURRENT_LIST_DIR}/../../roboeyes.cmake")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(roboeyes_headless)
