# Wokwi Simulation

This project includes a Wokwi setup at the repository root:

- `diagram.json` describes the ESP32-C3, ST7735 TFT, BME280, RTC, and five joystick buttons.
- `wokwi.toml` points Wokwi at PlatformIO's build outputs:
  - `.pio/build/esp32-c3-devkitc-02/firmware.bin`
  - `.pio/build/esp32-c3-devkitc-02/firmware.elf`

## Run

Build first:

```sh
pio run
```

Then start the simulator from VS Code with **Wokwi: Start Simulator**, or run a short headless check:

```sh
wokwi-cli . --timeout 12000 --timeout-exit-code 0
```

## Wiring Notes

The simulated wiring follows the README pin map:

| Function | ESP32-C3 GPIO | Wokwi pin |
| --- | ---: | --- |
| TFT MOSI | 6 | `esp:6` |
| TFT SCLK | 4 | `esp:4` |
| TFT CS | 7 | `esp:7` |
| TFT DC | 2 | `esp:2` |
| TFT RST | 1 | `esp:1` |
| I2C SDA | 8 | `esp:8` |
| I2C SCL | 9 | `esp:9` |
| Joystick Up | 0 | `esp:0` |
| Joystick Down | 5 | `esp:5` |
| Joystick Left | 10 | `esp:10` |
| Joystick Right | 20 | `esp:RX` |
| Joystick Click | 21 | `esp:TX` |

Wokwi's `board-esp32-c3-devkitm-1` labels GPIO20 and GPIO21 as `RX` and `TX`.
The firmware uses USB CDC for serial, so these pins are available for the joystick
in the simulation.

## Current Limits

- The ST7735 is represented by the `chip-st7735` custom chip dependency.
- The DS3231 is represented by Wokwi's `wokwi-ds1307` RTC. RTClib's basic
  `RTC_DS3231::begin()`, `now()`, and `adjust()` path works with the shared
  I2C address and compatible time registers.
- The local Wokwi CLI v0.26.1 does not recognize or emulate `wokwi-bme280`.
  A headless run boots, initializes the TFT and RTC, and reports
  `BME280 NOT FOUND`. Keep the BME280 in the diagram so the circuit mirrors the
  README wiring; use the firmware's partial hardware mode until Wokwi provides a
  compatible BME280 I2C part in the local runtime.
