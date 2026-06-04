# RoboEyes ST7789 ESP32-C3 Super Mini Example

This example runs RoboEyes on an ST7789 display.
Target board: ESP32-C3 Super Mini.

## Wiring

Use 3.3V logic only.
Do not connect LCD GPIO to 5V.
Leave LCD MISO disconnected.
Do not use GPIO2, GPIO8, or GPIO9 for LCD.

| LCD pin | SUPERMINI_C3 pin |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCL / SCK / CLK | GPIO6 |
| SDA / DIN / MOSI | GPIO7 |
| CS | GPIO10 |
| DC / A0 | GPIO4 |
| RST / RES | GPIO3 |
| BL / BLK / LED | GPIO5 |

## ESP-IDF

```bash
. "$IDF_PATH/export.sh"
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/cu.usbmodem314101 flash monitor
```

Press `Ctrl+]` to exit monitor.

## PlatformIO

```bash
pio run -e esp32c3
pio run -e esp32c3 -t upload --upload-port /dev/cu.usbmodem314101
pio device monitor -p /dev/cu.usbmodem314101 -b 115200
```

## Display Size

Default panel size in this example: 240x240.
Adjust `LCD_WIDTH`, `LCD_HEIGHT`, `LCD_GAP_X`, and `LCD_GAP_Y` in `main/main.cpp`
if your ST7789 panel uses different address offsets.

