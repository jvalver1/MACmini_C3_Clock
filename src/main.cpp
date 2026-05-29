#include "HardwareManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ScreenManager.h"
#include "SetupScreen.h"
#include "SplashScreen.h"
#include "SystemStatus.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// Global instances
TFT_eSPI tft = TFT_eSPI();
HardwareManager hw;
InputManager input;
NetworkManager net(hw);
ScreenManager *screenManager;

// Global sync primitives and variables definitions
SemaphoreHandle_t xStatusMutex = NULL;
QueueHandle_t xQueueInputEvents = NULL;
SystemStatus sysStatus;

// Task Function Declarations
void vUITask(void *pvParameters);
void vSensorInputTask(void *pvParameters);
void vNetworkTask(void *pvParameters);
void switchFromSplashWhenFinished();

constexpr unsigned long ENV_SENSOR_READ_INTERVAL_MS = 5UL * 60UL * 1000UL;

void setup() {
  // ESP32-C3 USB CDC — wait for the host to connect
  Serial.begin(115200);

  // Wait up to 5 seconds for USB CDC to be ready
  // Blink LED while waiting so user knows board is alive
  unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart < 5000)) {
    delay(100);
  }

  delay(300); // Extra settle time
  Serial.println();
  Serial.println("========================================");
  Serial.println("[BOOT] MACmini C3 Clock starting...");
  Serial.printf("[BOOT] USB CDC ready after %lu ms\n", millis() - waitStart);
  Serial.printf("[BOOT] Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("========================================");
  Serial.flush();

  // Create FreeRTOS synchronization primitives BEFORE starting tasks
  xStatusMutex = xSemaphoreCreateMutex();
  xQueueInputEvents = xQueueCreate(10, sizeof(ControlState));
  if (xStatusMutex == NULL || xQueueInputEvents == NULL) {
    Serial.println("[BOOT] FATAL ERROR: Failed to create FreeRTOS primitives!");
    while (1) { delay(1000); }
  }

  // Initialize Hardware (I2C sensors)
  Serial.println("[BOOT] Initializing hardware (I2C)...");
  Serial.flush();
  bool hwOk = hw.begin();
  Serial.printf("[BOOT] Hardware init: %s\n", hwOk ? "OK" : "PARTIAL/FAIL");
  Serial.flush();

  // Initialize Input
  Serial.println("[BOOT] Initializing input (joystick)...");
  Serial.flush();
  input.begin();
  Serial.println("[BOOT] Input ready.");
  Serial.flush();

  // Initialize Network & Preferences
  Serial.println("[BOOT] Initializing network & preferences...");
  Serial.flush();
  net.begin();
  Serial.println("[BOOT] Network ready.");
  Serial.flush();

  // Pre-populate System Status with initial readings
  if (xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
    sysStatus.currentTime = hw.getCurrentTime();
    sysStatus.envData = hw.getEnvironmentalData();
    sysStatus.wifiConnected = false;
    sysStatus.ntpSynced = false;
    sysStatus.weatherDesc = "";
    sysStatus.weatherTemp = 0.0f;
    sysStatus.weather = WeatherData();
    sysStatus.weatherValid = false;
    sysStatus.batteryVoltage = hw.getBatteryVoltage();
    xSemaphoreGive(xStatusMutex);
  }

  // Initialize TFT
  Serial.println("[BOOT] Initializing TFT display...");
  Serial.flush();
  tft.init();
  Serial.printf("[BOOT] TFT initialized. Width=%d, Height=%d\n", tft.width(),
                tft.height());
  Serial.flush();

  tft.setRotation(0);

  // --- TFT TEST PATTERN ---
  tft.fillScreen(TFT_RED);
  delay(200);
  tft.fillScreen(TFT_GREEN);
  delay(200);
  tft.fillScreen(TFT_BLUE);
  delay(200);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[BOOT] Test pattern done.");
  Serial.flush();

  // Draw boot message
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Booting...", 64, 80, 4);
  delay(300);

  // Initialize Screen Manager
  Serial.println("[BOOT] Creating ScreenManager...");
  Serial.flush();
  screenManager = new ScreenManager(tft, hw, net);

  // Launch FreeRTOS Tasks
  Serial.println("[BOOT] Spawning FreeRTOS tasks...");
  xTaskCreatePinnedToCore(vUITask, "UITask", 8192, NULL, 5, NULL, 0);
  xTaskCreatePinnedToCore(vSensorInputTask, "SensorInputTask", 2048, NULL, 6, NULL, 0);
  xTaskCreatePinnedToCore(vNetworkTask, "NetworkTask", 8192, NULL, 3, NULL, 0);

  Serial.println("[BOOT] Setup COMPLETE. Entering scheduler.");
  Serial.flush();
}

void loop() {
  // Bypassed: All CPU execution runs inside FreeRTOS tasks.
  // Yield the main loop forever to conserve CPU cycle scheduler overhead.
  vTaskDelay(portMAX_DELAY);
}

// ─── Task 1: UI & Render Task (Priority 5) ──────────────────────────────────
void vUITask(void *pvParameters) {
  Serial.println("[TASK] UI & Render task started.");
  
  while (true) {
    ControlState inputState;
    // Set default empty events
    inputState.joyUp = ButtonEvent::NONE;
    inputState.joyDown = ButtonEvent::NONE;
    inputState.joyLeft = ButtonEvent::NONE;
    inputState.joyRight = ButtonEvent::NONE;
    inputState.joyCenter = ButtonEvent::NONE;

    // Blockingly wait up to 20ms for input events from the Sensor/Input task
    if (xQueueReceive(xQueueInputEvents, &inputState, pdMS_TO_TICKS(20)) == pdTRUE) {
      bool isSplash = (screenManager->getCurrentType() == ScreenType::SPLASH);

      if (isSplash) {
        screenManager->handleInput(inputState);
        screenManager->update();
        switchFromSplashWhenFinished();
      } else {
        bool isSetup = (screenManager->getCurrentType() == ScreenType::SETUP);

        if (isSetup) {
          SetupScreen *setup =
              static_cast<SetupScreen *>(screenManager->getCurrentScreen());
          if (setup->isAtTopMenu() &&
              inputState.joyLeft == ButtonEvent::SHORT_PRESS) {
            screenManager->prevScreen();
            Serial.println("[NAV] Setup TOP JoyLeft SHORT -> Prev Screen");
          } else {
            screenManager->handleInput(inputState);
          }
        } else {
          if (inputState.joyRight == ButtonEvent::SHORT_PRESS) {
            screenManager->nextScreen();
            Serial.println("[NAV] JoyRight SHORT -> Next Screen");
          } else if (inputState.joyLeft == ButtonEvent::SHORT_PRESS) {
            screenManager->prevScreen();
            Serial.println("[NAV] JoyLeft SHORT -> Prev Screen");
          } else {
            screenManager->handleInput(inputState);
          }
        }
        screenManager->update();
      }
    } else {
      // Timeout triggered (no buttons pressed) -> run tick animations/seconds update
      screenManager->update();
      switchFromSplashWhenFinished();
    }
  }
}

void switchFromSplashWhenFinished() {
  if (screenManager->getCurrentType() != ScreenType::SPLASH) {
    return;
  }

  SplashScreen *splash =
      static_cast<SplashScreen *>(screenManager->getCurrentScreen());
  if (splash->isFinished()) {
    screenManager->setScreen(ScreenType::CLOCK);
    Serial.println("[NAV] Splash finished -> Clock screen");
  }
}

// ─── Task 2: Sensor & Input Task (Priority 6) ────────────────────────────────
void vSensorInputTask(void *pvParameters) {
  Serial.println("[TASK] Sensor & Input task started.");
  unsigned long lastSensorRead = 0;

  while (true) {
    // 1. Poll joystick switches
    ControlState inputState = input.update();
    bool hasEvent = (inputState.joyUp != ButtonEvent::NONE ||
                     inputState.joyDown != ButtonEvent::NONE ||
                     inputState.joyLeft != ButtonEvent::NONE ||
                     inputState.joyRight != ButtonEvent::NONE ||
                     inputState.joyCenter != ButtonEvent::NONE);
    if (hasEvent) {
      xQueueSend(xQueueInputEvents, &inputState, 0);
    }

    // 2. Poll RTC Time (every 100ms for sub-second precision on page redraws)
    DateTime nowTime = hw.getCurrentTime();

    // 3. Poll environmental BME280 sensor (every 5 minutes)
    unsigned long nowMs = millis();
    bool readSensors =
        (nowMs - lastSensorRead >= ENV_SENSOR_READ_INTERVAL_MS ||
         lastSensorRead == 0);
    EnvironmentalData env = {0, 0, 0, false};
    if (readSensors) {
      env = hw.getEnvironmentalData();
      lastSensorRead = nowMs;
    }

    // Write to shared memory under mutex lock
    if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
      sysStatus.currentTime = nowTime;
      if (readSensors && env.valid) {
        sysStatus.envData = env;
      }
      sysStatus.batteryVoltage = hw.getBatteryVoltage();
      xSemaphoreGive(xStatusMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // Yield to run at 50Hz polling rate
  }
}

// ─── Task 3: Background Network Task (Priority 3) ───────────────────────────
void vNetworkTask(void *pvParameters) {
  Serial.println("[TASK] Asynchronous background Network task started.");

  // Attempt initial network connection asynchronously
  Serial.println("[TASK] Network: Connecting WiFi...");
  bool connected = net.connect();
  
  if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
    sysStatus.wifiConnected = connected;
    xSemaphoreGive(xStatusMutex);
  }

  if (connected) {
    Serial.println("[TASK] Network: WiFi connected, syncing NTP time...");
    bool synced = net.syncTime();
    if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
      sysStatus.ntpSynced = synced;
      xSemaphoreGive(xStatusMutex);
    }
  }

  unsigned long lastWeatherFetch = 0;
  const unsigned long weatherIntervalMs = 10UL * 60UL * 1000UL;
  const unsigned long weatherRetryMs = 60UL * 1000UL;

  while (true) {
    // Keep WiFi connection alive
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    if (!currentlyConnected) {
      Serial.println("[TASK] Network: WiFi connection lost, reconnecting...");
      currentlyConnected = net.connect();
    }

    if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
      sysStatus.wifiConnected = currentlyConnected;
      xSemaphoreGive(xStatusMutex);
    }

    // Fetch weather updates from API (every 10 minutes)
    unsigned long nowMs = millis();
    if (currentlyConnected &&
        (nowMs - lastWeatherFetch >= weatherIntervalMs ||
         lastWeatherFetch == 0)) {
      WeatherData weather;
      Serial.println("[TASK] Network: Fetching weather forecast...");
      bool weatherOk = net.fetchWeather(weather);

      if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, portMAX_DELAY) == pdTRUE) {
        if (weatherOk) {
          sysStatus.weather = weather;
          sysStatus.weatherDesc = weather.description;
          sysStatus.weatherTemp = weather.temperature;
          sysStatus.weatherValid = true;
        } else {
          // Zambretti fallback handled on WeatherScreen.cpp
          sysStatus.weatherValid = false;
        }
        xSemaphoreGive(xStatusMutex);
      }
      lastWeatherFetch =
          weatherOk ? nowMs : nowMs - (weatherIntervalMs - weatherRetryMs);
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Yield to run connection diagnostics every 5 seconds
  }
}
