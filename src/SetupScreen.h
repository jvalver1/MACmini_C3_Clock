#ifndef SETUP_SCREEN_H
#define SETUP_SCREEN_H

#include "KeyboardView.h"
#include "NetworkManager.h"
#include "ScreenManager.h"

// Two-level setup menu states
enum class SetupState {
  TOP_MENU,   // WiFi Setup | Set Time | Set Date
  WIFI_MENU,  // SSID | Password | City | Save & Reboot
  SSID_LIST,  // Scanned WiFi networks list
  ENTER_SSID, // Keyboard: manual SSID entry
  ENTER_PASS, // Keyboard: password entry
  ENTER_CITY  // Keyboard: city entry
};

#define MAX_SSID_LIST 10

class SetupScreen : public Screen {
public:
  SetupScreen(TFT_eSPI &tft, HardwareManager &hw, NetworkManager &net);
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;
  bool isAtTopMenu() const { return _state == SetupState::TOP_MENU; }

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  NetworkManager &_net;
  KeyboardView _kb;

  SetupState _state;
  int _topIdx;        // Top menu cursor
  int _wifiIdx;       // WiFi sub-menu cursor
  int _ssidIdx;       // SSID list cursor
  int _ssidCount;     // Number of scanned SSIDs
  int _ssidScrollTop; // First visible SSID in the list
  String _ssidList[MAX_SSID_LIST];

  ConfigData _pendingConfig;
  bool _needsRedraw;

  // Toast state
  String _toastMsg;
  unsigned long _toastExpiry;

  void drawTopMenu(TFT_eSPI &tft);
  void drawWifiMenu(TFT_eSPI &tft);
  void drawSsidList(TFT_eSPI &tft);
  void startScan();
};

#endif // SETUP_SCREEN_H
