#include "HardwareManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "ScreenManager.h"
#include "SplashScreen.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

// Global instances
TFT_eSPI tft = TFT_eSPI();
HardwareManager hw;
InputManager input;
NetworkManager net(hw);
ScreenManager *screenManager;

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

  // Attempt connectivity at boot
  Serial.println("[BOOT] Attempting WiFi connection...");
  Serial.flush();
  if (net.connect()) {
    Serial.println("[BOOT] WiFi CONNECTED!");
    Serial.printf("[BOOT] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("[BOOT] Syncing NTP time...");
    bool ntpOk = net.syncTime();
    if (ntpOk) {
      Serial.println("[BOOT] Time source: NTP -> RTC updated.");
    } else {
      Serial.println("[BOOT] NTP sync failed, using existing RTC time.");
    }
  } else {
    Serial.println(
        "[BOOT] WiFi connection FAILED (offline mode, using RTC time).");
  }
  // Log current time regardless of source
  DateTime bootTime = hw.getCurrentTime();
  Serial.printf("[BOOT] Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                bootTime.year(), bootTime.month(), bootTime.day(),
                bootTime.hour(), bootTime.minute(), bootTime.second());
  Serial.flush();

  // Initialize TFT
  Serial.println("[BOOT] Initializing TFT display...");
#ifdef CONFIG_IDF_TARGET_ESP32C3
  Serial.println("[BOOT] Target: ESP32-C3 (FSPI expected)");
#else
  Serial.println("[BOOT] WARNING: ESP32-C3 NOT detected by IDF!");
#endif
#ifdef USE_FSPI_PORT
  Serial.println("[BOOT] USE_FSPI_PORT is set");
#endif
  Serial.flush();
  tft.init();
  Serial.printf("[BOOT] TFT initialized. Width=%d, Height=%d\n", tft.width(),
                tft.height());
  Serial.flush();

  tft.setRotation(0);

  // --- TFT TEST PATTERN ---
  tft.fillScreen(TFT_RED);
  delay(400);
  tft.fillScreen(TFT_GREEN);
  delay(400);
  tft.fillScreen(TFT_BLUE);
  delay(400);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[BOOT] Test pattern done.");
  Serial.flush();

  // Draw boot message
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Booting...", 64, 80, 4);
  delay(500);

  // Initialize Screen Manager
  Serial.println("[BOOT] Creating ScreenManager...");
  Serial.flush();
  screenManager = new ScreenManager(tft, hw, net);
  Serial.println("[BOOT] Setup COMPLETE. Entering main loop.");
  Serial.flush();

  Serial.println("[BOOT] Boot finished successfully.");
}

unsigned long lastLoopLog = 0;

void loop() {
  ControlState inputState = input.update();

  // Check if splash screen is active
  bool isSplash = (screenManager->getCurrentType() == ScreenType::SPLASH);

  if (isSplash) {
    SplashScreen *splash =
        static_cast<SplashScreen *>(screenManager->getCurrentScreen());

    // Splash screen handles joyCenter internally for transitions
    screenManager->handleInput(inputState);
    screenManager->update();

    // Once splash signals finished, go directly to Clock screen
    if (splash->isFinished()) {
      screenManager->setScreen(ScreenType::CLOCK);
      Serial.println("[NAV] Splash finished -> Clock screen");
    }
  } else {
    // Normal navigation for all other screens
    if (inputState.joyRight == ButtonEvent::SHORT_PRESS) {
      screenManager->nextScreen();
      Serial.println("[NAV] JoyRight SHORT -> Next Screen");
    } else if (inputState.joyLeft == ButtonEvent::SHORT_PRESS) {
      screenManager->prevScreen();
      Serial.println("[NAV] JoyLeft SHORT -> Prev Screen");
    } else if (inputState.joyCenter == ButtonEvent::LONG_PRESS) {
      screenManager->setScreen(ScreenType::SETUP);
      Serial.println("[NAV] JoyCenter LONG -> Setup");
    } else {
      // Only forward input to the screen when NOT navigating,
      // to prevent the press from leaking into the new screen
      screenManager->handleInput(inputState);
    }
    screenManager->update();
  }

  // Heartbeat every 3 seconds for debugging
  unsigned long now = millis();
  if (now - lastLoopLog > 3000) {
    Serial.printf("[LOOP] t=%lus heap=%u\n", now / 1000, ESP.getFreeHeap());
    Serial.flush();
    lastLoopLog = now;
  }

  delay(10);
}
