#include "KeyboardView.h"

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

KeyboardView::KeyboardView(TFT_eSPI &tft)
    : _tft(tft), _text(""), _cursorX(0), _cursorY(0), _page(0),
      _finished(false) {}

void KeyboardView::handleInput(const ControlState &state) {
  // 5-Way Stick Navigation
  if (state.joyRight == ButtonEvent::SHORT_PRESS) {
    _cursorX++;
    if (_cursorX > 5)
      _cursorX = 0;
  }
  if (state.joyLeft == ButtonEvent::SHORT_PRESS) {
    _cursorX--;
    if (_cursorX < 0)
      _cursorX = 5;
  }
  if (state.joyDown == ButtonEvent::SHORT_PRESS) {
    _cursorY++;
    if (_cursorY > 4)
      _cursorY = 0;
  }
  if (state.joyUp == ButtonEvent::SHORT_PRESS) {
    _cursorY--;
    if (_cursorY < 0)
      _cursorY = 4;
  }

  // Page Toggle (Joystick Up long press)
  if (state.joyUp == ButtonEvent::LONG_PRESS) {
    _page = (_page + 1) % 3;
  }

  // Select Character (Joy Center short press)
  if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
    int idx = _cursorY * 6 + _cursorX;
    if (idx < (int)strlen(pages[_page])) {
      _text += pages[_page][idx];
    }
  }

  // Backspace (Joystick Left long press)
  if (state.joyLeft == ButtonEvent::LONG_PRESS) {
    if (_text.length() > 0) {
      _text.remove(_text.length() - 1);
    }
  }

  // Done (Joy Center long press)
  if (state.joyCenter == ButtonEvent::LONG_PRESS) {
    _finished = true;
  }
}

void KeyboardView::draw() {
  drawMidnightGradient(_tft);
  drawTextBar();
  drawGrid();

  // Draw Legend
  _tft.setTextColor(0x7BEF); // Darker gray
  _tft.setTextDatum(BC_DATUM);
  _tft.drawString("JoyU+:Page JoyL+:Del", 64, 145, 1);

  char pageLabel[16];
  sprintf(pageLabel, "PAGE %d/3", _page + 1);
  _tft.drawString(pageLabel, 64, 155, 1);
}

void KeyboardView::drawTextBar() {
  _tft.drawRect(5, 5, 118, 25, 0xC618);
  _tft.setTextColor(TFT_WHITE);
  _tft.setTextDatum(ML_DATUM);
  String display = _text + "_";
  if (display.length() > 18)
    display = display.substring(display.length() - 18);
  _tft.drawString(display, 10, 17, 2);
}

void KeyboardView::drawGrid() {
  int startY = 40;
  int cellW = 20;
  int cellH = 20;

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 6; x++) {
      int idx = y * 6 + x;
      int px = 5 + x * cellW;
      int py = startY + y * cellH;

      if (x == _cursorX && y == _cursorY) {
        _tft.fillRect(px, py, cellW - 2, cellH - 2, 0x03FF); // Electric Blue
        _tft.setTextColor(TFT_BLACK);
      } else {
        _tft.drawRect(px, py, cellW - 2, cellH - 2, 0x2104);
        _tft.setTextColor(TFT_WHITE);
      }

      if (idx < (int)strlen(pages[_page])) {
        char c[2] = {pages[_page][idx], '\0'};
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString(c, px + cellW / 2 - 1, py + cellH / 2 - 1, 2);
      }
    }
  }
}
