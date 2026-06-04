# RoboEyes Headless ESP-IDF Example

This example runs RoboEyes without display wiring.
It renders frames and logs sampled checksums.

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

