#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "HardwareManager.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

class Screen; // Forward declaration

enum class ScreenType { SPLASH = 0, CLOCK, ENV, WEATHER, MOON, SETUP, COUNT };

class Screen {
public:
  virtual void onEntry() {}
  virtual void onExit() {}
  virtual void update(unsigned long now) = 0;
  virtual void draw(TFT_eSPI &tft) = 0;
  virtual void handleInput(const ControlState &state) = 0;
  virtual ~Screen() {}
};

class ScreenManager {
public:
  ScreenManager(TFT_eSPI &tft, HardwareManager &hw, NetworkManager &net);
  void setScreen(ScreenType type);
  void update();
  void handleInput(const ControlState &state);
  void nextScreen();
  void prevScreen();
  Screen *getCurrentScreen() const { return currentScreen; }
  ScreenType getCurrentType() const { return currentType; }

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  NetworkManager &_net;
  Screen *currentScreen;
  ScreenType currentType;
};

#endif // SCREEN_MANAGER_H
