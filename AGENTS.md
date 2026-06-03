# RoboEyes Instructions

## Core Rules

- Always add unit tests for new or changed logic.
- Keep code modular. Use focused components, structs, classes, and funcs.
- Avoid dumping logic into one file.
- Use dependency injection where practical.
- Avoid duplicated logic. Extract shared helpers.
- Keep files ASCII unless file already uses non-ASCII.
- Respect local changes. Never revert unrelated user work.

## Project Shape

- `components/eyes`: platform-light animation, renderer, player, sequencer, GIF helpers.
- `components/eyes_emotion_cycle`: reusable demo cycle runner.
- `components/eyes_lcd_sink`: LCD frame sink for rendered eyes frames.
- `components/lcd_st7789`: ST7789 SPI display wrapper.
- `components/pixel_convert`: pixel conversion helpers.
- `tools/eyes_gif_runner`: host preview GIF runner.
- `roboeyes.cmake`: ESP-IDF import helper.

## ESP-IDF Rules

- Keep RoboEyes usable as copied code, submodule, or subtree.
- Keep `roboeyes.cmake` working before `project()` in downstream ESP-IDF apps.
- Components must declare every public ESP-IDF dependency in `REQUIRES`.
- Components including `driver/gpio.h` must require `esp_driver_gpio`.
- Keep platform-specific code in sink/display components.
- Keep `components/eyes` portable enough for host GIF tools.

## Tests And Builds

- Run host GIF tests after changing eyes rendering or GIF output.
- Run component unit tests after changing framework component behavior.
- In downstream integration, run available script tests and full firmware builds.
- If working inside standalone RoboEyes, run available host build/test commands:
  - `cmake -S tools/eyes_gif_runner -B build-host-roboeyes_gif_runner`
  - `cmake --build build-host-roboeyes_gif_runner --parallel 4`
  - `build-host-roboeyes_gif_runner/eyes_gif_runner --out out/eyes_gifs --emotion neutral --duration-ms 240`

## Documentation

- Update `README.md` when API, component layout, import flow, or test flow changes.
- Keep examples copy-pasteable.
- Document firmware wiring with concrete CMake and C++ snippets.

## User Preferences

- Finish operational path. Do not stop at diagnosis.
- Verify with tests and builds before final response.
- Use Obsidian for project maps when working inside parent projects that use it.
- Prefer conservative, reversible changes.
- Prove safety before deleting or moving important local state.

## Writing Style

- Write caveman.
- Facts first.
- Short sentences.
- One thought per line where useful.
- No em dash.
- No filler.
- Keep user-facing product copy normal when needed.
