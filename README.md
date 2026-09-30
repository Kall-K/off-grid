# off-grid

Experiments for the LilyGO T3-S3 LoRa board, built with PlatformIO and the Arduino framework. The project includes simple hardware checks for the onboard LED and OLED display, plus an SX1262 LoRa transmit/receive demo for two devices.

## Included Sketches

| Sketch | Purpose |
| --- | --- |
| `codes/blink_test.cpp` | Blinks the onboard LED once per second and reports button presses and board details over serial. |
| `codes/display.cpp` | Exercises the OLED. A single button press cycles pages; a double press toggles LED blinking. |
| `codes/off_grid.cpp` | SX1262 LoRa TX/RX test with OLED status, battery reading, RSSI, and SNR reporting. |

The files in `codes/` are standalone Arduino sketches. PlatformIO compiles `src/main.cpp`, so copy the sketch you want to flash into that path before building.

## Libraries

Project libraries are declared in `platformio.ini` and are installed automatically by PlatformIO:

- RadioLib
- Adafruit SSD1306
- Adafruit GFX Library

## LoRa Demo

`codes/off_grid.cpp` is designed for two boards. Before flashing, set the `ID` constant near the top of the sketch:

- Set one board to `ID 0`: it sends a `Hello <number>` packet when the BOOT button is pressed.
- Set the other board to `ID 1`: it sends a packet automatically every 15 seconds.
- Both boards listen for packets, flash the LED on activity, and show TX/RX counts, signal metrics, battery voltage, and uptime on the OLED.

The demo is configured for 868 MHz, SF9, 125 kHz bandwidth, coding rate 4/7, and 10 dBm output power. Adjust the frequency and radio settings for your hardware and region, and operate only within applicable local radio regulations.


