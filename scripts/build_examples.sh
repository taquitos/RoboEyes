#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="${BUILD_DIR:-$repo_dir/build-host-roboeyes_gif_runner}"
cmake_cmd="${CMAKE:-cmake}"
jobs="${JOBS:-4}"
skip_pio="${SKIP_PIO:-0}"

if [ "$skip_pio" != "1" ]; then
  if [ "${PIO:-}" ]; then
    pio_cmd=$PIO
  elif command -v pio >/dev/null 2>&1; then
    pio_cmd=pio
  elif [ -x "$HOME/.platformio/penv/bin/pio" ]; then
    pio_cmd="$HOME/.platformio/penv/bin/pio"
  else
    echo "PlatformIO not found. Set PIO=/path/to/pio or SKIP_PIO=1." >&2
    exit 127
  fi
fi

run_pio() {
  example_dir=$1
  env_name=$2

  echo "==> PlatformIO $example_dir $env_name"
  (
    cd "$repo_dir/$example_dir"
    "$pio_cmd" run -e "$env_name"
  )
}

echo "==> CMake tools/eyes_gif_runner"
"$cmake_cmd" -S "$repo_dir/tools/eyes_gif_runner" -B "$build_dir"
"$cmake_cmd" --build "$build_dir" --parallel "$jobs"

if [ "$skip_pio" = "1" ]; then
  echo "==> SKIP_PIO=1, skipping PlatformIO examples"
else
  run_pio examples/espidf_headless esp32c3
  run_pio examples/espidf_st7789_supermini_c3 esp32c3
  run_pio examples/espidf_st7789_tinys3 tinys3
fi
