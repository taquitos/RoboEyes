#!/bin/sh
set -eu

usage() {
  echo "Usage: $0 OUTPUT_DIR [headless|st7789-supermini-c3|st7789-tinys3]" >&2
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  usage
  exit 2
fi

out_dir=$1
profile=${2:-headless}

case "$profile" in
  headless)
    preset=headless
    board=esp32-c3-devkitm-1
    env_name=esp32c3
    idf_target=esp32c3
    flash_size=4MB
    max_size=4194304
    project_name=roboeyes_headless
    requires="eyes eyes_emotion_cycle eyes_log_sink"
    clip_options='
  options.size = {128, 64};
  options.fps = 20;
  options.realtime = false;
'
    main_body='
  eyes_log::EyesLogSinkConfig sink_cfg;
  sink_cfg.tag = TAG;
  eyes_log::EyesLogSink sink(sink_cfg);
'
    includes='#include "eyes_log_sink.h"'
    ;;
  st7789-supermini-c3)
    preset=st7789
    board=esp32-c3-devkitm-1
    env_name=esp32c3
    idf_target=esp32c3
    flash_size=4MB
    max_size=4194304
    project_name=roboeyes_st7789_supermini_c3
    requires="eyes eyes_emotion_cycle eyes_lcd_sink lcd_st7789 pixel_convert"
    clip_options='
  options.size = {lcd_cfg.width, lcd_cfg.height};
  options.fps = 30;
  options.realtime = true;
'
    main_body='
  const lcd::St7789Config lcd_cfg = lcd::supermini_c3_st7789_config_240x240();
  lcd::St7789Display display;
  ESP_ERROR_CHECK(display.init(lcd_cfg));

  eyes_lcd::EyesLcdSinkConfig sink_cfg;
  sink_cfg.display = &display;
  eyes_lcd::EyesLcdSink sink(sink_cfg);
'
    includes='#include "esp_check.h"
#include "eyes_lcd_sink.h"
#include "lcd_st7789.h"'
    ;;
  st7789-tinys3)
    preset=st7789
    board=um_tinys3
    env_name=tinys3
    idf_target=esp32s3
    flash_size=8MB
    max_size=8388608
    project_name=roboeyes_st7789_tinys3
    requires="eyes eyes_emotion_cycle eyes_lcd_sink lcd_st7789 pixel_convert"
    clip_options='
  options.size = {lcd_cfg.width, lcd_cfg.height};
  options.fps = 30;
  options.realtime = true;
'
    main_body='
  const lcd::St7789Config lcd_cfg = lcd::tinys3_st7789v3_config_240x280();
  lcd::St7789Display display;
  ESP_ERROR_CHECK(display.init(lcd_cfg));

  eyes_lcd::EyesLcdSinkConfig sink_cfg;
  sink_cfg.display = &display;
  eyes_lcd::EyesLcdSink sink(sink_cfg);
'
    includes='#include "esp_check.h"
#include "eyes_lcd_sink.h"
#include "lcd_st7789.h"'
    ;;
  *)
    usage
    exit 2
    ;;
esac

mkdir -p "$out_dir/main" "$out_dir/frameworks"

cat > "$out_dir/.gitignore" <<'EOF'
.pio/
build/
managed_components/
sdkconfig
sdkconfig.*
dependencies.lock
EOF

cat > "$out_dir/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.16)

set(ROBOEYES_PRESET $preset)
include("\${CMAKE_CURRENT_LIST_DIR}/frameworks/RoboEyes/roboeyes.cmake")

include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project($project_name)
EOF

cat > "$out_dir/main/CMakeLists.txt" <<EOF
idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "."
  REQUIRES $requires
)
EOF

cat > "$out_dir/main/main.cpp" <<EOF
#include "esp_log.h"

#include "eyes/eyes.h"
#include "eyes_emotion_cycle.h"
$includes

namespace {
constexpr char TAG[] = "$project_name";
} // namespace

extern "C" void app_main() {
  ESP_LOGI(TAG, "RoboEyes boot");
$main_body
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions options;
$clip_options
  if (!eyes_cycle::run_forever(player, options)) {
    ESP_LOGE(TAG, "Emotion cycle stopped");
  }
}
EOF

cat > "$out_dir/platformio.ini" <<EOF
[platformio]
src_dir = main

[env:$env_name]
platform = espressif32
board = $board
framework = espidf
monitor_speed = 115200
board_build.flash_size = $flash_size
board_upload.flash_size = $flash_size
board_upload.maximum_size = $max_size
EOF

cat > "$out_dir/sdkconfig.defaults" <<EOF
CONFIG_IDF_TARGET="$idf_target"
CONFIG_ESPTOOLPY_FLASHSIZE_$flash_size=y
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
EOF

cat > "$out_dir/README.md" <<EOF
# $project_name

Generated RoboEyes project.

Add RoboEyes at \`frameworks/RoboEyes\`:

\`\`\`bash
git submodule add git@github.com:taquitos/RoboEyes.git frameworks/RoboEyes
\`\`\`

Build:

\`\`\`bash
pio run -e $env_name
\`\`\`
EOF

echo "Created $profile project in $out_dir"
