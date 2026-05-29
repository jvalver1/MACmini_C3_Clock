#include "SetupScreen.h"

namespace {
inline uint16_t HW_COLOR(uint8_t r, uint8_t g, uint8_t b) {
  return (((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

uint16_t blendColor(uint16_t a, uint16_t b, uint8_t amount) {
  uint8_t ar = ((a & 0x001F) << 3);
  uint8_t ag = ((a & 0x07E0) >> 3);
  uint8_t ab = ((a & 0xF800) >> 8);
  uint8_t br = ((b & 0x001F) << 3);
  uint8_t bg = ((b & 0x07E0) >> 3);
  uint8_t bb = ((b & 0xF800) >> 8);

  uint8_t r = ar + (((int)br - ar) * amount) / 255;
  uint8_t g = ag + (((int)bg - ag) * amount) / 255;
  uint8_t blue = ab + (((int)bb - ab) * amount) / 255;
  return HW_COLOR(r, g, blue);
}

void drawMidnightGradient(TFT_eSPI &tft) {
  for (int y = 0; y < 160; y++) {
    uint8_t amount = (uint8_t)((y * 255) / 159);
    uint16_t color =
        blendColor(HW_COLOR(0, 2, 14), HW_COLOR(5, 18, 48), amount);
    tft.drawFastHLine(0, y, 128, color);
  }
}
} // namespace

SetupScreen::SetupScreen(TFT_eSPI &tft, HardwareManager &hw,
                         NetworkManager &net)
    : _tft(tft), _hw(hw), _net(net), _kb(tft), _state(SetupState::TOP_MENU),
      _topIdx(0), _wifiIdx(0), _ssidIdx(0), _ssidCount(0), _ssidScrollTop(0),
      _needsRedraw(true), _toastExpiry(0) {
  _pendingConfig = _net.loadConfig();
}

// ── Update ──────────────────────────────────────────────────────────────────

void SetupScreen::update(unsigned long now) {
  // Clear expired toast
  if (_toastExpiry > 0 && now >= _toastExpiry) {
    _toastExpiry = 0;
    _toastMsg = "";
    _needsRedraw = true;
  }

  // Keyboard finished?
  if ((_state == SetupState::ENTER_SSID || _state == SetupState::ENTER_PASS ||
       _state == SetupState::ENTER_CITY) &&
      _kb.isFinished()) {

    if (_state == SetupState::ENTER_SSID) {
      _pendingConfig.ssid = _kb.getText();
      _state = SetupState::WIFI_MENU;
    } else if (_state == SetupState::ENTER_PASS) {
      _pendingConfig.pass = _kb.getText();
      _state = SetupState::WIFI_MENU;
    } else if (_state == SetupState::ENTER_CITY) {
      _pendingConfig.cityName = _kb.getText();
      _state = SetupState::WIFI_MENU;
    }
    _kb.reset();
    _needsRedraw = true;
  }
}

// ── Draw ────────────────────────────────────────────────────────────────────

void SetupScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  switch (_state) {
  case SetupState::TOP_MENU:
    drawTopMenu(tft);
    break;
  case SetupState::WIFI_MENU:
    drawWifiMenu(tft);
    break;
  case SetupState::SSID_LIST:
    drawSsidList(tft);
    break;
  case SetupState::ENTER_SSID:
  case SetupState::ENTER_PASS:
  case SetupState::ENTER_CITY:
    _kb.draw();
    tft.setTextColor(0xC618);
    tft.setTextDatum(TC_DATUM);
    if (_state == SetupState::ENTER_SSID)
      tft.drawString("ENTER SSID", 64, 30, 1);
    if (_state == SetupState::ENTER_PASS)
      tft.drawString("ENTER PASSWORD", 64, 30, 1);
    if (_state == SetupState::ENTER_CITY)
      tft.drawString("ENTER CITY", 64, 30, 1);
    break;
  }

  _needsRedraw = false;
}

// ── Draw: Top Menu ──────────────────────────────────────────────────────────

void SetupScreen::drawTopMenu(TFT_eSPI &tft) {
  drawMidnightGradient(tft);
  tft.setTextDatum(TC_DATUM);

  // Title
  tft.setTextColor(0x07FF); // Cyan title
  tft.drawString("SETUP", 64, 8, 2);

  // Separator line
  tft.drawFastHLine(10, 26, 108, 0x4208);

  const char *items[] = {"WiFi Setup", "Set Time", "Set Date"};
  for (int i = 0; i < 3; i++) {
    int py = 38 + i * 30;
    if (i == _topIdx) {
      tft.fillRoundRect(8, py - 2, 112, 24, 3, 0x03FF);
      tft.setTextColor(TFT_BLACK);
    } else {
      tft.setTextColor(TFT_WHITE);
    }
    tft.drawString(items[i], 64, py + 2, 2);
  }

  // Toast message (if any)
  if (_toastMsg.length() > 0) {
    tft.fillRoundRect(10, 120, 108, 20, 3, 0x7BEF);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(_toastMsg, 64, 130, 1);
  }

  // Legend
  tft.setTextColor(0x7BEF);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("U/D:Nav  Click:Select", 64, 155, 1);
}

// ── Draw: WiFi Sub-Menu ─────────────────────────────────────────────────────

void SetupScreen::drawWifiMenu(TFT_eSPI &tft) {
  drawMidnightGradient(tft);
  tft.setTextDatum(TC_DATUM);

  // Title
  tft.setTextColor(0x07FF);
  tft.drawString("WIFI SETUP", 64, 8, 2);

  tft.drawFastHLine(10, 26, 108, 0x4208);

  const char *items[] = {"SSID", "Password", "City", "SAVE & REBOOT"};
  // Show current values as preview
  String previews[] = {_pendingConfig.ssid, "****", _pendingConfig.cityName,
                       ""};

  for (int i = 0; i < 4; i++) {
    int py = 34 + i * 28;
    if (i == _wifiIdx) {
      tft.fillRoundRect(8, py - 2, 112, 24, 3, 0x03FF);
      tft.setTextColor(TFT_BLACK);
    } else {
      tft.setTextColor(TFT_WHITE);
    }
    tft.drawString(items[i], 64, py + 2, 2);

    // Show preview underneath (small gray text)
    if (previews[i].length() > 0) {
      tft.setTextColor(i == _wifiIdx ? 0x2104 : 0x7BEF);
      String preview = previews[i];
      if (preview.length() > 16)
        preview = preview.substring(0, 16) + "..";
      tft.drawString(preview, 64, py + 16, 1);
    }
  }

  // Legend
  tft.setTextColor(0x7BEF);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("U/D:Nav Click:Sel L:Back", 64, 155, 1);
}

// ── Draw: SSID List ─────────────────────────────────────────────────────────

void SetupScreen::drawSsidList(TFT_eSPI &tft) {
  drawMidnightGradient(tft);
  tft.setTextDatum(TC_DATUM);

  tft.setTextColor(0x07FF);
  tft.drawString("SELECT NETWORK", 64, 5, 2);
  tft.drawFastHLine(10, 22, 108, 0x4208);

  if (_ssidCount == 0) {
    tft.setTextColor(0x7BEF);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("No networks found", 64, 80, 2);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("L:Back", 64, 155, 1);
    return;
  }

  // Total items = scanned networks + "Manual entry" option
  int totalItems = _ssidCount + 1;
  int maxVisible = 5;

  for (int v = 0; v < maxVisible && (_ssidScrollTop + v) < totalItems; v++) {
    int itemIdx = _ssidScrollTop + v;
    int py = 28 + v * 24;

    bool isManual = (itemIdx == _ssidCount); // Last item = Manual entry
    bool selected = (itemIdx == _ssidIdx);

    if (selected) {
      tft.fillRoundRect(8, py - 2, 112, 20, 3, 0x03FF);
      tft.setTextColor(TFT_BLACK);
    } else {
      tft.setTextColor(TFT_WHITE);
    }

    if (isManual) {
      tft.drawString("[Manual entry]", 64, py + 1, 1);
    } else {
      String ssid = _ssidList[itemIdx];
      if (ssid.length() > 18)
        ssid = ssid.substring(0, 18);
      tft.drawString(ssid, 64, py + 1, 1);
    }
  }

  // Scroll indicator
  if (totalItems > maxVisible) {
    tft.setTextColor(0x7BEF);
    tft.setTextDatum(BC_DATUM);
    char scrollInfo[16];
    sprintf(scrollInfo, "%d/%d", _ssidIdx + 1, totalItems);
    tft.drawString(scrollInfo, 64, 148, 1);
  }

  tft.setTextColor(0x7BEF);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("U/D:Nav Click:Sel L:Back", 64, 158, 1);
}

// ── WiFi Scan ───────────────────────────────────────────────────────────────

void SetupScreen::startScan() {
  // Show "Scanning..." message immediately
  drawMidnightGradient(_tft);
  _tft.setTextColor(0x07FF);
  _tft.setTextDatum(MC_DATUM);
  _tft.drawString("Scanning...", 64, 70, 2);
  _tft.setTextColor(0x7BEF);
  _tft.drawString("Please wait", 64, 95, 1);

  // Blocking scan
  _ssidCount = _net.scanNetworks(_ssidList, MAX_SSID_LIST);
  _ssidIdx = 0;
  _ssidScrollTop = 0;
  Serial.printf("[SETUP] Scan complete: %d networks\n", _ssidCount);
}

// ── Input ───────────────────────────────────────────────────────────────────

void SetupScreen::handleInput(const ControlState &state) {

  // ── TOP MENU ──────────────────────────────────────────────────────────────
  if (_state == SetupState::TOP_MENU) {
    if (state.joyDown == ButtonEvent::SHORT_PRESS) {
      _topIdx = (_topIdx + 1) % 3;
      _needsRedraw = true;
    }
    if (state.joyUp == ButtonEvent::SHORT_PRESS) {
      _topIdx = (_topIdx - 1 + 3) % 3;
      _needsRedraw = true;
    }
    if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
      if (_topIdx == 0) {
        // WiFi Setup
        _state = SetupState::WIFI_MENU;
        _wifiIdx = 0;
        _needsRedraw = true;
      } else {
        // Set Time / Set Date → placeholder toast
        _toastMsg = "Coming soon...";
        _toastExpiry = millis() + 2000;
        _needsRedraw = true;
      }
    }
    return;
  }

  // ── WIFI SUB-MENU ─────────────────────────────────────────────────────────
  if (_state == SetupState::WIFI_MENU) {
    if (state.joyDown == ButtonEvent::SHORT_PRESS) {
      _wifiIdx = (_wifiIdx + 1) % 4;
      _needsRedraw = true;
    }
    if (state.joyUp == ButtonEvent::SHORT_PRESS) {
      _wifiIdx = (_wifiIdx - 1 + 4) % 4;
      _needsRedraw = true;
    }
    // Back to top menu
    if (state.joyLeft == ButtonEvent::SHORT_PRESS) {
      _state = SetupState::TOP_MENU;
      _needsRedraw = true;
      return;
    }
    if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
      if (_wifiIdx == 0) {
        // SSID → scan then show list
        _state = SetupState::SSID_LIST;
        startScan();
        _needsRedraw = true;
      } else if (_wifiIdx == 1) {
        // Password → keyboard
        _state = SetupState::ENTER_PASS;
        _kb.setText(_pendingConfig.pass);
        _needsRedraw = true;
      } else if (_wifiIdx == 2) {
        // City → keyboard
        _state = SetupState::ENTER_CITY;
        _kb.setText(_pendingConfig.cityName);
        _needsRedraw = true;
      } else if (_wifiIdx == 3) {
        // Save & Reboot
        _net.saveConfig(_pendingConfig);
        ESP.restart();
      }
    }
    return;
  }

  // ── SSID LIST ─────────────────────────────────────────────────────────────
  if (_state == SetupState::SSID_LIST) {
    int totalItems = _ssidCount + 1; // networks + "Manual entry"
    int maxVisible = 5;

    if (state.joyDown == ButtonEvent::SHORT_PRESS) {
      _ssidIdx = (_ssidIdx + 1) % totalItems;
      // Adjust scroll window
      if (_ssidIdx >= _ssidScrollTop + maxVisible) {
        _ssidScrollTop = _ssidIdx - maxVisible + 1;
      }
      if (_ssidIdx == 0)
        _ssidScrollTop = 0; // Wrapped
      _needsRedraw = true;
    }
    if (state.joyUp == ButtonEvent::SHORT_PRESS) {
      _ssidIdx = (_ssidIdx - 1 + totalItems) % totalItems;
      if (_ssidIdx < _ssidScrollTop) {
        _ssidScrollTop = _ssidIdx;
      }
      if (_ssidIdx == totalItems - 1) { // Wrapped to end
        _ssidScrollTop = max(0, totalItems - maxVisible);
      }
      _needsRedraw = true;
    }
    // Back to WiFi menu
    if (state.joyLeft == ButtonEvent::SHORT_PRESS) {
      _state = SetupState::WIFI_MENU;
      _needsRedraw = true;
      return;
    }
    if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
      if (_ssidIdx == _ssidCount) {
        // Manual entry → keyboard
        _state = SetupState::ENTER_SSID;
        _kb.setText(_pendingConfig.ssid);
      } else {
        // Picked from list
        _pendingConfig.ssid = _ssidList[_ssidIdx];
        _state = SetupState::WIFI_MENU;
        Serial.printf("[SETUP] SSID selected: %s\n",
                      _pendingConfig.ssid.c_str());
      }
      _needsRedraw = true;
    }
    return;
  }

  // ── KEYBOARD STATES ───────────────────────────────────────────────────────
  _kb.handleInput(state);
  _needsRedraw = true;
}
