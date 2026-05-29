#include "MoonScreen.h"

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

  drawMidnightGradient(tft);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("MOON PHASE", 64, 15, 2);

  // Draw the Moon icon sized to fill the available space
  // Determine waxing (first half of lunar cycle) vs waning
  bool waxing = (_moon.age < 14.765f);
  Icons::drawMoon(tft, 64, 80, 70, 0xE71C, _moon.illumination, waxing);

  // Phase Name
  tft.setTextColor(0x03FF); // Electric Blue
  tft.drawString(_moon.phaseName, 64, 125, 2);

  // Illumination
  tft.setTextColor(0xC618);
  char illumBuf[16];
  sprintf(illumBuf, "Illum: %.0f%%", _moon.illumination * 100);
  tft.drawString(illumBuf, 64, 145, 1);

  _needsRedraw = false;
}

void MoonScreen::handleInput(const ControlState &state) {}
