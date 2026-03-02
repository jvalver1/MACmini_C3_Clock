#include "SetupScreen.h"

SetupScreen::SetupScreen(TFT_eSPI &tft, HardwareManager &hw,
                         NetworkManager &net)
    : _tft(tft), _hw(hw), _net(net), _kb(tft), _state(SetupState::MENU),
      _menuIdx(0), _needsRedraw(true) {
  _pendingConfig = _net.loadConfig();
}

void SetupScreen::update(unsigned long now) {
  if (_state != SetupState::MENU && _kb.isFinished()) {
    if (_state == SetupState::ENTER_SSID) {
      _pendingConfig.ssid = _kb.getText();
      _state = SetupState::ENTER_PASS;
      _kb.reset();
      _kb.setText(_pendingConfig.pass);
    } else if (_state == SetupState::ENTER_PASS) {
      _pendingConfig.pass = _kb.getText();
      _state = SetupState::MENU;
      _kb.reset();
    } else if (_state == SetupState::ENTER_CITY) {
      _pendingConfig.cityName = _kb.getText();
      _state = SetupState::MENU;
      _kb.reset();
    }
    _needsRedraw = true;
  }
}

void SetupScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  if (_state == SetupState::MENU) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SETUP MENU", 64, 10, 2);

    const char *menuItems[] = {"WiFi SSID", "WiFi Pass", "City Search",
                               "SAVE & REBOOT"};
    for (int i = 0; i < 4; i++) {
      int py = 40 + i * 30;
      if (i == _menuIdx) {
        tft.fillRect(5, py - 2, 118, 25, 0x03FF);
        tft.setTextColor(TFT_BLACK);
      } else {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      tft.drawString(menuItems[i], 64, py, 2);
    }

    // Legend at bottom
    tft.setTextColor(0x7BEF, TFT_BLACK);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("Joy:Nav  Click:Select", 64, 155, 1);
  } else {
    _kb.draw();
    tft.setTextColor(0xC618, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    if (_state == SetupState::ENTER_SSID)
      tft.drawString("ENTER SSID", 64, 30, 1);
    if (_state == SetupState::ENTER_PASS)
      tft.drawString("ENTER PASSWORD", 64, 30, 1);
    if (_state == SetupState::ENTER_CITY)
      tft.drawString("ENTER CITY", 64, 30, 1);
  }

  _needsRedraw = false;
}

void SetupScreen::handleInput(const ControlState &state) {
  if (_state == SetupState::MENU) {
    if (state.joyDown == ButtonEvent::SHORT_PRESS) {
      _menuIdx = (_menuIdx + 1) % 4;
      _needsRedraw = true;
    }
    if (state.joyUp == ButtonEvent::SHORT_PRESS) {
      _menuIdx = (_menuIdx - 1 + 4) % 4;
      _needsRedraw = true;
    }

    if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
      if (_menuIdx == 0) {
        _state = SetupState::ENTER_SSID;
        _kb.setText(_pendingConfig.ssid);
      } else if (_menuIdx == 1) {
        _state = SetupState::ENTER_PASS;
        _kb.setText(_pendingConfig.pass);
      } else if (_menuIdx == 2) {
        _state = SetupState::ENTER_CITY;
        _kb.setText(_pendingConfig.cityName);
      } else if (_menuIdx == 3) {
        _net.saveConfig(_pendingConfig);
        ESP.restart();
      }
      _needsRedraw = true;
    }

    // Setup exit is handled by main.cpp (joyCenter long press)
  } else {
    _kb.handleInput(state);
    _needsRedraw = true;
  }
}
