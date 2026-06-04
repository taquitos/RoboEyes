# RoboEyes ST7789 TinyS3 Example

This example runs RoboEyes on an ST7789V3 display.
Target board: Unexpected Maker TinyS3.
Panel: ST7789V3, 240x280.

Firmware rotates the panel 90 degrees:
`swap_xy=true`, `mirror_y=true`, `gap_x=20`.

## Wiring

Use 3.3V logic only.
Do not connect LCD GPIO to 5V.
Leave LCD MISO disconnected.

| LCD pin | TinyS3 GPIO |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCL / SCK / CLK | GPIO36 |
| SDA / DIN / MOSI | GPIO35 |
| CS | GPIO34 |
| DC / A0 | GPIO7 |
| RST / RES | GPIO6 |
| BL / BLK / LED | GPIO21 |

## ESP-IDF

```bash
. "$IDF_PATH/export.sh"
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

Press `Ctrl+]` to exit monitor.

## PlatformIO

```bash
pio run -e tinys3
pio run -e tinys3 -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor -p /dev/cu.usbmodemXXXX -b 115200
```

