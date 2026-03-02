#include "MoonScreen.h"

MoonScreen::MoonScreen(TFT_eSPI &tft, HardwareManager &hw)
    : _tft(tft), _hw(hw), _needsRedraw(true), _lastAge(-1.0f) {}

void MoonScreen::update(unsigned long now) {
  MoonData current = MoonPhase::calculate(_hw.getCurrentTime());

  // Only trigger redraw when the moon age changes by at least 0.01 days
  // (~14 minutes), avoiding unnecessary redraws while still catching real
  // changes
  if (_lastAge < 0 || fabs(current.age - _lastAge) > 0.01f) {
    _moon = current;
    _lastAge = current.age;
    _needsRedraw = true;
  }
}

void MoonScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("MOON PHASE", 64, 15, 2);

  // Draw the Moon icon sized to fill the available space
  // Determine waxing (first half of lunar cycle) vs waning
  bool waxing = (_moon.age < 14.765f);
  Icons::drawMoon(tft, 64, 80, 70, 0xE71C, _moon.illumination, waxing);

  // Phase Name
  tft.setTextColor(0x03FF, TFT_BLACK); // Electric Blue
  tft.drawString(_moon.phaseName, 64, 125, 2);

  // Illumination
  tft.setTextColor(0xC618, TFT_BLACK);
  char illumBuf[16];
  sprintf(illumBuf, "Illum: %.0f%%", _moon.illumination * 100);
  tft.drawString(illumBuf, 64, 145, 1);

  _needsRedraw = false;
}

void MoonScreen::handleInput(const ControlState &state) {}
