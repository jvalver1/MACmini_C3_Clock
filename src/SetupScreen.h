#ifndef SETUP_SCREEN_H
#define SETUP_SCREEN_H

#include "KeyboardView.h"
#include "NetworkManager.h"
#include "ScreenManager.h"


enum class SetupState { MENU, ENTER_SSID, ENTER_PASS, ENTER_CITY, SAVING };

class SetupScreen : public Screen {
public:
  SetupScreen(TFT_eSPI &tft, HardwareManager &hw, NetworkManager &net);
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  NetworkManager &_net;
  KeyboardView _kb;

  SetupState _state;
  int _menuIdx;
  ConfigData _pendingConfig;

  bool _needsRedraw;
};

#endif // SETUP_SCREEN_H
