#include "EnvScreen.h"

EnvScreen::EnvScreen(TFT_eSPI &tft, HardwareManager &hw)
    : _tft(tft), _hw(hw), _lastUpdate(0), _needsRedraw(true) {}

void EnvScreen::update(unsigned long now) {
  if (now - _lastUpdate > 2000) { // Every 2 seconds
    _data = _hw.getEnvironmentalData();
    _lastUpdate = now;
    _needsRedraw = true;
  }
}

void EnvScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  tft.fillScreen(TFT_BLACK);

  // Header
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("ENVIRONMENT", 64, 10, 2);

  // Temp Card
  tft.drawRoundRect(10, 35, 108, 50, 4, 0x2104); // Dark Gray border
  tft.setTextColor(0xC618, TFT_BLACK);
  tft.drawString("TEMP", 64, 40, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char tempBuf[16];
  sprintf(tempBuf, "%.1f C", _data.temperature);
  tft.drawString(tempBuf, 64, 58, 4);

  // Humidity Card
  tft.drawRoundRect(10, 92, 52, 40, 4, 0x2104);
  tft.setTextColor(0xC618, TFT_BLACK);
  tft.setCursor(15, 97);
  tft.print("HUM");
  tft.setTextColor(0x03FF, TFT_BLACK);
  char humBuf[16];
  sprintf(humBuf, "%.0f%%", _data.humidity);
  tft.setCursor(15, 112);
  tft.print(humBuf);

  // Pressure Card
  tft.drawRoundRect(66, 92, 52, 40, 4, 0x2104);
  tft.setTextColor(0xC618, TFT_BLACK);
  tft.setCursor(71, 97);
  tft.print("PRES");
  tft.setTextColor(0x03FF, TFT_BLACK);
  char presBuf[16];
  sprintf(presBuf, "%.0f", _data.pressure);
  tft.setCursor(71, 112);
  tft.print(presBuf);

  // Thermometer icon — positioned to the right of the header, not overlapping
  // the temp value
  Icons::drawThermometer(tft, 110, 15, 16, 0xFD00);

  _needsRedraw = false;
}

void EnvScreen::handleInput(const ControlState &state) {}
