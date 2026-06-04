# RoboEyes

Reusable ESP-IDF eyes animation framework.

## Emotion Gallery

| Neutral | Happiness | Curiosity |
| --- | --- | --- |
| <img src="docs/emotions/neutral.gif" width="180" alt="Neutral"> | <img src="docs/emotions/happiness.gif" width="180" alt="Happiness"> | <img src="docs/emotions/curiosity.gif" width="180" alt="Curiosity"> |

| Exhaustion | Falling Asleep | Waking Up |
| --- | --- | --- |
| <img src="docs/emotions/exhaustion.gif" width="180" alt="Exhaustion"> | <img src="docs/emotions/falling_asleep.gif" width="180" alt="Falling Asleep"> | <img src="docs/emotions/waking_up.gif" width="180" alt="Waking Up"> |

| Snoring | Dizzy | Playful Pong |
| --- | --- | --- |
| <img src="docs/emotions/snoring.gif" width="180" alt="Snoring"> | <img src="docs/emotions/dizzy.gif" width="180" alt="Dizzy"> | <img src="docs/emotions/playful_pong.gif" width="180" alt="Playful Pong"> |

## Components

- `eyes`: platform-light core animation, renderer, sequencer, GIF helpers.
- `eyes_emotion_cycle`: ready-made emotion cycle runner for firmware demos.
- `eyes_log_sink`: log-based frame sink for headless firmware smoke tests.
- `eyes_lcd_sink`: frame sink that sends rendered grayscale frames to LCD.
- `lcd_st7789`: ST7789 SPI display wrapper.
- `pixel_convert`: grayscale-to-RGB565 conversion helper.

## Use In ESP-IDF Project

Copy or submodule this `RoboEyes` directory, then include its helper before
`project()` in your root `CMakeLists.txt`:

```cmake
include("${CMAKE_CURRENT_LIST_DIR}/frameworks/RoboEyes/roboeyes.cmake")
```

The helper includes all RoboEyes components by default. Apps can include only
selected components by setting `ROBOEYES_COMPONENTS` before the helper:

```cmake
set(ROBOEYES_COMPONENTS eyes eyes_emotion_cycle eyes_log_sink)
include("${CMAKE_CURRENT_LIST_DIR}/frameworks/RoboEyes/roboeyes.cmake")
```

Apps that need ST7789 output need `eyes`, `eyes_lcd_sink`, `lcd_st7789`, and
`pixel_convert`.

## Firmware Wiring

Create display, sink, animator, renderer, and player:

```cpp
lcd::St7789Display display;
ESP_ERROR_CHECK(display.init(lcd_cfg));

eyes::EyesAnimator animator;
eyes::EyesRenderer renderer;

eyes_lcd::EyesLcdSinkConfig sink_cfg = {
    .display = &display,
};
eyes_lcd::EyesLcdSink sink(sink_cfg);

eyes::EyesClipPlayer player(animator, renderer, sink);
eyes::ClipOptions opt;
opt.size = {lcd_cfg.width, lcd_cfg.height};
opt.fps = 30;
opt.realtime = true;
```

Play one emotion:

```cpp
player.play(eyes::Emotion::Happiness, opt);
```

Run built-in demo cycle:

```cpp
eyes_cycle::run_forever(player, opt);
```

## Host GIF Tool

Build preview GIFs without ESP-IDF:

```bash
cmake -S tools/eyes_gif_runner -B build-host-roboeyes_gif_runner
cmake --build build-host-roboeyes_gif_runner --parallel 4
build-host-roboeyes_gif_runner/eyes_gif_runner --out out/eyes_gifs --emotion all
```

## Tests

- Component tests live beside framework components.
- Host GIF behavior can be checked with the host GIF tool.
- Downstream ESP-IDF apps should run their own component test and build suites.

## License

MIT. See `LICENSE`.
