# MACmini C3 Clock

A feature-rich desktop clock and environmental station built on the **ESP32-C3** microcontroller with a 1.8" colour TFT display. The device provides real-time clock, local weather sensing, online weather forecasts, moon phase tracking, and a fully on-device setup menu — all driven by a five-way joystick switch.

---

## ✨ Features

| Screen          | Description                                                                                                                                                                             |
| --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Clock**       | Displays current time and date sourced from the DS3231 RTC. Time is synchronised via NTP on every boot when WiFi is available.                                                          |
| **Environment** | Live temperature, humidity, and barometric pressure readings from the BME280 sensor, refreshed periodically.                                                                            |
| **Weather**     | Dual-mode weather information: **online** forecasts via a weather API when connected, or **offline** local trend prediction using the Zambretti algorithm based on barometric pressure. |
| **Moon Phase**  | Calculates and displays the current lunar phase, illumination percentage, and moon age using an astronomical algorithm — no internet required.                                          |
| **Setup**       | On-device configuration menu with an on-screen keyboard for entering WiFi SSID, password, and city name. Settings are persisted in flash memory (Preferences).                          |

### Additional Capabilities

- **NTP Time Sync** — Automatically synchronises the RTC with `pool.ntp.org` at startup.
- **Persistent Configuration** — WiFi credentials and city are stored in ESP32 flash (NVS) and survive reboots.
- **Zambretti Weather Prediction** — Offline barometric weather forecasting algorithm for when WiFi is unavailable.
- **Modular Architecture** — Clean separation of concerns: Hardware, Input, Network, Screen, and Display layers.

---

## 🔧 Hardware

### Components

| Component               | Description                                              |
| ----------------------- | -------------------------------------------------------- |
| **ESP32-C3-DevKitC-02** | Main microcontroller (RISC-V, WiFi, USB CDC)             |
| **1.8" ST7735 TFT**     | 128×160 colour display (SPI)                             |
| **BME280**              | Temperature, humidity & pressure sensor (I2C)            |
| **DS3231**              | High-precision real-time clock with battery backup (I2C) |
| **5-Way Joystick**      | Sole input device — directional + centre click           |

### Pin Mapping — ESP32-C3

#### SPI — TFT Display (ST7735)

| Signal | GPIO  | Notes          |
| ------ | ----- | -------------- |
| MOSI   | **6** | SPI data out   |
| SCLK   | **4** | SPI clock      |
| CS     | **7** | Chip select    |
| DC     | **2** | Data / Command |
| RST    | **1** | Reset          |

#### I2C — Sensors & RTC

| Signal | GPIO  | Notes                             |
| ------ | ----- | --------------------------------- |
| SDA    | **8** | Shared I2C data (BME280 + DS3231) |
| SCL    | **9** | Shared I2C clock                  |

> **BME280 address**: auto-detected at `0x76` or `0x77`.

#### 5-Way Joystick Switch

| Direction    | GPIO   | Notes                                    |
| ------------ | ------ | ---------------------------------------- |
| Up           | **0**  |                                          |
| Down         | **5**  |                                          |
| Left         | **10** |                                          |
| Right        | **20** | Short press = next screen                |
| Centre Click | **21** | Short press = select, Long press = Setup |

---

## 🏗️ Wiring Diagram

```
ESP32-C3-DevKitC-02
┌──────────────────────┐
│                      │
│  GPIO 6  (MOSI) ────────── TFT MOSI
│  GPIO 4  (SCLK) ────────── TFT SCK
│  GPIO 7  (CS)   ────────── TFT CS
│  GPIO 2  (DC)   ────────── TFT DC
│  GPIO 1  (RST)  ────────── TFT RST
│                      │
│  GPIO 8  (SDA)  ────────┬─ BME280 SDA
│                      │  └─ DS3231 SDA
│  GPIO 9  (SCL)  ────────┬─ BME280 SCL
│                      │  └─ DS3231 SCL
│                      │
│  GPIO 0  ────────────────── Joystick UP
│  GPIO 5  ────────────────── Joystick DOWN
│  GPIO 10 ────────────────── Joystick LEFT
│  GPIO 20 ────────────────── Joystick RIGHT
│  GPIO 21 ────────────────── Joystick CLICK
│                      │
│  3V3 ────────────────────── VCC (TFT, BME280, DS3231)
│  GND ────────────────────── GND (all modules)
│                      │
└──────────────────────┘
```

---

## 📂 Project Structure

```
MACmini_C3_Clock/
├── platformio.ini          # Build config, library deps & TFT_eSPI flags
├── include/
│   └── config.h            # WiFi, NTP & API key configuration
├── src/
│   ├── main.cpp            # Entry point — init & main loop
│   ├── Pins.h              # All GPIO pin definitions
│   │
│   ├── HardwareManager.*   # BME280 + DS3231 initialisation & readings
│   ├── InputManager.*      # Joystick debouncing with short/long press
│   ├── NetworkManager.*    # WiFi, NTP sync, API calls & Preferences storage
│   ├── ScreenManager.*     # Screen lifecycle, navigation (next/prev)
│   │
│   ├── ClockScreen.*       # Time & date display
│   ├── EnvScreen.*         # Temperature, humidity & pressure display
│   ├── WeatherScreen.*     # Online forecast + Zambretti offline prediction
│   ├── MoonScreen.*        # Lunar phase display
│   ├── SetupScreen.*       # On-device WiFi & city configuration
│   │
│   ├── KeyboardView.*      # On-screen keyboard (lower/upper/symbols)
│   ├── Zambretti.h         # Zambretti barometric forecasting algorithm
│   ├── MoonPhase.h         # Astronomical moon phase calculator
│   └── Icons.h             # Drawable weather & UI icons
└── test/
```

---

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ESP32-C3-DevKitC-02 board
- USB-C cable (native USB CDC — no external UART chip needed)

### Build & Upload

```bash
# Clone the repository
git clone https://github.com/<your-username>/MACmini_C3_Clock.git
cd MACmini_C3_Clock

# Build
pio run

# Upload
pio run --target upload

# Open Serial Monitor (115200 baud)
pio device monitor
```

### Configuration

Edit `include/config.h` before building:

```c
#define WIFI_SSID     "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
#define NTP_SERVER    "pool.ntp.org"
#define GMT_OFFSET_SEC 0       // e.g. 3600 for CET
#define DAYLIGHT_OFFSET_SEC 0  // e.g. 3600 for summer time
```

> Alternatively, use the **Setup screen** on the device to enter WiFi credentials and city — these are stored persistently and override the compile-time defaults.

---

## 📚 Libraries

| Library                                                                | Version | Purpose                                 |
| ---------------------------------------------------------------------- | ------- | --------------------------------------- |
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)                         | ^2.5.43 | TFT display driver (ST7735)             |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson)                | ^7.2.0  | JSON parsing for API responses          |
| [Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library) | ^2.2.4  | Temperature, humidity & pressure sensor |
| [RTClib](https://github.com/adafruit/RTClib)                           | ^2.1.3  | DS3231 real-time clock                  |
| [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor) | ^1.1.14 | Sensor abstraction (BME280 dependency)  |

---

## 🧭 Navigation

| Input                           | Action                   |
| ------------------------------- | ------------------------ |
| **Joystick right** short press  | Cycle to next screen     |
| **Joystick left** short press   | Cycle to previous screen |
| **Joystick centre** long press  | Enter Setup screen       |
| **Joystick centre** short press | Select item / character  |
| **Joystick up / down**          | Navigate menus in Setup  |

Screen order: **Splash → Clock → Environment → Weather → Moon** (wraps around). Setup is accessed via centre long press only.

### On-Screen Keyboard Controls

| Input                     | Action                            |
| ------------------------- | --------------------------------- |
| **Joystick** directions   | Move cursor                       |
| **Joystick centre** short | Type character                    |
| **Joystick centre** long  | Done / confirm                    |
| **Joystick up** long      | Toggle page (a-z / A-Z / symbols) |
| **Joystick left** long    | Backspace                         |

---

## 📝 License

This project is provided as-is for personal and educational use.

---

## 🙏 Acknowledgements

- [Espressif ESP32-C3](https://www.espressif.com/en/products/socs/esp32-c3) — RISC-V WiFi SoC
- [Zambretti Algorithm](https://en.wikipedia.org/wiki/Zambretti_Forecaster) — Barometric weather prediction
- [PlatformIO](https://platformio.org/) — Embedded development platform
