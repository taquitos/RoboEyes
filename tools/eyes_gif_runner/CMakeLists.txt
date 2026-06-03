cmake_minimum_required(VERSION 3.16)

project(eyes_gif_runner CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(eyes_gif_runner
  main.cpp
  ../../components/eyes/src/clock.cpp
  ../../components/eyes/src/animator.cpp
  ../../components/eyes/src/emotion.cpp
  ../../components/eyes/src/pixel_buffer.cpp
  ../../components/eyes/src/player.cpp
  ../../components/eyes/src/sequencer.cpp
  ../../components/eyes/src/transition.cpp
  ../../components/eyes/src/renderer.cpp
  ../../components/eyes/src/gif/encoder.cpp
  ../../components/eyes/src/gif/file_byte_sink.cpp
  ../../components/eyes/src/gif/gif_frame_sink.cpp
  ../../components/eyes/src/gif/vector_byte_sink.cpp
)

target_include_directories(eyes_gif_runner PRIVATE
  ../../components/eyes/include
)
