# RoboEyes Examples

## Headless

Path: `examples/espidf_headless`
Target: any ESP32 supported by ESP-IDF.
Output: serial logs with sampled frame checksums.
Use this first when checking toolchain and framework integration.

```bash
cd examples/espidf_headless
pio run -e esp32c3
```

## ESP32-C3 Super Mini ST7789

Path: `examples/espidf_st7789_supermini_c3`
Target: ESP32-C3 Super Mini.
Panel: ST7789, default 240x240.
MISO unused.
Use 3.3V logic only.
Do not use GPIO2, GPIO8, or GPIO9 for LCD.

```bash
cd examples/espidf_st7789_supermini_c3
pio run -e esp32c3
```

## TinyS3 ST7789V3

Path: `examples/espidf_st7789_tinys3`
Target: Unexpected Maker TinyS3.
Panel: ST7789V3, 240x280.
Rotation: `swap_xy=true`, `mirror_y=true`, `gap_x=20`.
MISO unused.
Use 3.3V logic only.

```bash
cd examples/espidf_st7789_tinys3
pio run -e tinys3
```

