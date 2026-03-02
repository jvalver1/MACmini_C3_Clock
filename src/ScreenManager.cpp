#include "ScreenManager.h"
#include "ClockScreen.h"
#include "EnvScreen.h"
#include "MoonScreen.h"
#include "SetupScreen.h"
#include "SplashScreen.h"
#include "WeatherScreen.h"

ScreenManager::ScreenManager(TFT_eSPI &tft, HardwareManager &hw,
                             NetworkManager &net)
    : _tft(tft), _hw(hw), _net(net), currentScreen(nullptr),
      currentType(ScreenType::SPLASH) {
  Serial.println("[SCR] ScreenManager constructor");
  setScreen(ScreenType::SPLASH);
}

void ScreenManager::setScreen(ScreenType type) {
  Serial.printf("[SCR] setScreen(%d)\n", (int)type);

  if (currentScreen) {
    currentScreen->onExit();
    delete currentScreen;
    currentScreen = nullptr;
    Serial.println("[SCR] Previous screen deleted.");
  }

  currentType = type;

  // Clear the screen BEFORE creating the new screen so onEntry() draws on a
  // clean canvas
  _tft.fillScreen(TFT_BLACK);
  Serial.println("[SCR] fillScreen(BLACK) done.");

  switch (type) {
  case ScreenType::SPLASH:
    Serial.println("[SCR] Creating SplashScreen...");
    currentScreen = new SplashScreen(_tft);
    break;
  case ScreenType::CLOCK:
    Serial.println("[SCR] Creating ClockScreen...");
    currentScreen = new ClockScreen(_tft, _hw);
    break;
  case ScreenType::ENV:
    Serial.println("[SCR] Creating EnvScreen...");
    currentScreen = new EnvScreen(_tft, _hw);
    break;
  case ScreenType::WEATHER:
    Serial.println("[SCR] Creating WeatherScreen...");
    currentScreen = new WeatherScreen(_tft, _hw, _net);
    break;
  case ScreenType::MOON:
    Serial.println("[SCR] Creating MoonScreen...");
    currentScreen = new MoonScreen(_tft, _hw);
    break;
  case ScreenType::SETUP:
    Serial.println("[SCR] Creating SetupScreen...");
    currentScreen = new SetupScreen(_tft, _hw, _net);
    break;
  default:
    Serial.printf("[SCR] WARNING: Unknown type %d, defaulting to Clock\n",
                  (int)type);
    currentScreen = new ClockScreen(_tft, _hw);
    break;
  }

  if (currentScreen) {
    Serial.println("[SCR] Calling onEntry()...");
    currentScreen->onEntry();
    Serial.println("[SCR] onEntry() done. Screen is live.");
  } else {
    Serial.println("[SCR] ERROR: currentScreen is NULL!");
  }
}

void ScreenManager::update() {
  if (currentScreen) {
    currentScreen->update(millis());
    currentScreen->draw(_tft);
  }
}

void ScreenManager::handleInput(const ControlState &state) {
  if (currentScreen) {
    currentScreen->handleInput(state);
  }
}

void ScreenManager::nextScreen() {
  // Cycle through display screens: CLOCK -> ENV -> WEATHER -> MOON -> CLOCK
  // SPLASH is only shown at boot; SETUP is only accessed via long press
  int next = (int)currentType + 1;
  if (next >= (int)ScreenType::SETUP) {
    next = (int)ScreenType::CLOCK; // Wrap back to CLOCK
  }
  // Skip SPLASH if somehow we land on it (shouldn't happen, but guard)
  if (next == (int)ScreenType::SPLASH) {
    next = (int)ScreenType::CLOCK;
  }
  Serial.printf("[SCR] nextScreen: %d -> %d\n", (int)currentType, next);
  setScreen((ScreenType)next);
}

void ScreenManager::prevScreen() {
  // Cycle backwards through display screens: CLOCK <- ENV <- WEATHER <- MOON <-
  // CLOCK SPLASH is only shown at boot; SETUP is only accessed via long press
  int prev = (int)currentType - 1;
  if (prev <= (int)ScreenType::SPLASH) {
    prev = (int)ScreenType::MOON; // Wrap to last display screen
  }
  Serial.printf("[SCR] prevScreen: %d -> %d\n", (int)currentType, prev);
  setScreen((ScreenType)prev);
}
