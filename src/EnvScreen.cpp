#include "EnvScreen.h"
#include "SystemStatus.h"
#include <math.h>
#include <string.h>

namespace {
inline uint16_t HW_COLOR(uint8_t r, uint8_t g, uint8_t b) {
  return (((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

constexpr unsigned long ENV_STATUS_SAMPLE_MS = 1000;
constexpr float TEMP_REDRAW_DELTA = 0.05f;
constexpr float HUM_REDRAW_DELTA = 0.5f;
constexpr float PRESSURE_REDRAW_DELTA = 0.5f;

const uint16_t COL_PANEL = HW_COLOR(4, 7, 28);
const uint16_t COL_WHITE = HW_COLOR(250, 250, 246);
const uint16_t COL_TEMP_A = HW_COLOR(255, 47, 44);
const uint16_t COL_TEMP_B = HW_COLOR(255, 137, 22);
const uint16_t COL_TEMP_C = HW_COLOR(255, 232, 30);
const uint16_t COL_HUM = HW_COLOR(28, 176, 255);
const uint16_t COL_HUM_2 = HW_COLOR(54, 221, 255);
const uint16_t COL_PRESS = HW_COLOR(100, 221, 42);
const uint16_t COL_PRESS_2 = HW_COLOR(154, 255, 34);
const uint16_t COL_ORANGE = HW_COLOR(255, 118, 18);

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

const char *glyph5x7(char c) {
  switch (c) {
  case '0': return "111101101101101101111";
  case '1': return "010110010010010010111";
  case '2': return "111001001111100100111";
  case '3': return "111001001111001001111";
  case '4': return "101101101111001001001";
  case '5': return "111100100111001001111";
  case '6': return "111100100111101101111";
  case '7': return "111001001010010010010";
  case '8': return "111101101111101101111";
  case '9': return "111101101111001001111";
  case '.': return "000000000000000000010";
  case ':': return "000010000000010000000";
  case '%': return "101001010010010100101";
  case 'A': return "010101101111101101101";
  case 'B': return "110101101110101101110";
  case 'C': return "111100100100100100111";
  case 'D': return "110101101101101101110";
  case 'E': return "111100100111100100111";
  case 'F': return "111100100111100100100";
  case 'H': return "101101101111101101101";
  case 'I': return "111010010010010010111";
  case 'M': return "101111111101101101101";
  case 'N': return "101111111111111111101";
  case 'O': return "111101101101101101111";
  case 'P': return "111101101111100100100";
  case 'R': return "110101101110101101101";
  case 'S': return "111100100111001001111";
  case 'T': return "111010010010010010010";
  case 'U': return "101101101101101101111";
  case 'Y': return "101101101010010010010";
  case 'a': return "000000111001111101111";
  case 'h': return "100100110101101101101";
  default: return "000000000000000000000";
  }
}

void drawPixelText(TFT_eSprite &spr, const char *text, int x, int y, int scale,
                   uint16_t color, bool centered = false) {
  int len = strlen(text);
  int charW = 3 * scale;
  int gap = scale;
  int totalW = len * charW + (len - 1) * gap;
  if (centered) {
    x -= totalW / 2;
  }

  for (int i = 0; i < len; i++) {
    if (text[i] == ' ') {
      x += charW + gap;
      continue;
    }
    const char *g = glyph5x7(text[i]);
    for (int gy = 0; gy < 7; gy++) {
      for (int gx = 0; gx < 3; gx++) {
        if (g[gy * 3 + gx] == '1') {
          spr.fillRect(x + gx * scale, y + gy * scale, scale, scale, color);
        }
      }
    }
    x += charW + gap;
  }
}

int pixelTextWidth(const char *text, int scale) {
  int len = strlen(text);
  if (len == 0) {
    return 0;
  }
  return len * 3 * scale + (len - 1) * scale;
}

void drawReading(TFT_eSprite &spr, const char *value, const char *unit, int x,
                 int y, int valueScale, int unitScale, uint16_t valueColor,
                 uint16_t unitColor, bool degreeSymbol) {
  int valueW = pixelTextWidth(value, valueScale);
  drawPixelText(spr, value, x, y, valueScale, valueColor);
  x += valueW + valueScale;

  if (degreeSymbol) {
    spr.drawCircle(x + 1, y + 3, 2, unitColor);
    x += 6;
  }

  int unitY = y + (7 * valueScale - 7 * unitScale);
  drawPixelText(spr, unit, x, unitY, unitScale, unitColor);
}

bool changedEnough(const EnvironmentalData &oldData,
                   const EnvironmentalData &newData) {
  if (!oldData.valid || oldData.valid != newData.valid) {
    return true;
  }

  return fabsf(oldData.temperature - newData.temperature) >= TEMP_REDRAW_DELTA ||
         fabsf(oldData.humidity - newData.humidity) >= HUM_REDRAW_DELTA ||
         fabsf(oldData.pressure - newData.pressure) >= PRESSURE_REDRAW_DELTA;
}
} // namespace

EnvScreen::EnvScreen(TFT_eSPI &tft, HardwareManager &hw)
    : _tft(tft), _hw(hw), _time(DateTime(2026, 1, 1, 0, 0, 0)),
      _screenSprite(&tft), _lastUpdate(0), _lastMinute(-1),
      _needsRedraw(true), _spriteCreated(false) {
  _data = {0, 0, 0, false};
}

EnvScreen::~EnvScreen() { deleteSprite(); }

void EnvScreen::onEntry() {
  createSprite();
  _lastUpdate = 0;
  _lastMinute = -1;
  _needsRedraw = true;
}

void EnvScreen::onExit() { deleteSprite(); }

void EnvScreen::createSprite() {
  if (_spriteCreated) {
    return;
  }
  _screenSprite.setColorDepth(16);
  _screenSprite.createSprite(128, 160);
  _spriteCreated = true;
}

void EnvScreen::deleteSprite() {
  if (!_spriteCreated) {
    return;
  }
  _screenSprite.deleteSprite();
  _spriteCreated = false;
}

void EnvScreen::update(unsigned long now) {
  if (now - _lastUpdate < ENV_STATUS_SAMPLE_MS && _lastUpdate != 0) {
    return;
  }

  EnvironmentalData freshData = {0, 0, 0, false};
  DateTime freshTime = _time;
  bool dataCopied = false;
  if (xStatusMutex != NULL &&
      xSemaphoreTake(xStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    freshData = sysStatus.envData;
    freshTime = sysStatus.currentTime;
    xSemaphoreGive(xStatusMutex);
    dataCopied = true;
  }

  if (dataCopied) {
    if (freshTime.minute() != _lastMinute) {
      _time = freshTime;
      _lastMinute = freshTime.minute();
      _needsRedraw = true;
    }

    if (freshData.valid && changedEnough(_data, freshData)) {
      _data = freshData;
      _needsRedraw = true;
    }
  }

  _lastUpdate = now;
}

void EnvScreen::drawGaugeArc(TFT_eSprite &spr, int cx, int cy, int radius,
                             int startDeg, int endDeg, uint16_t color,
                             int thickness) {
  for (int deg = startDeg; deg <= endDeg; deg += 3) {
    float rad = deg * DEG_TO_RAD;
    int x = cx + (int)roundf(cosf(rad) * radius);
    int y = cy + (int)roundf(sinf(rad) * radius);
    spr.fillRect(x - thickness / 2, y - thickness / 2, thickness, thickness,
                 color);
  }
}

void EnvScreen::drawBackground(TFT_eSprite &spr) {
  for (int y = 0; y < 160; y++) {
    uint8_t amount = (uint8_t)((y * 255) / 159);
    uint16_t color =
        blendColor(HW_COLOR(0, 2, 14), HW_COLOR(5, 18, 48), amount);
    spr.drawFastHLine(0, y, 128, color);
  }
}

void EnvScreen::drawThermometerIcon(TFT_eSprite &spr, int x, int y,
                                    uint16_t color) {
  spr.drawRoundRect(x - 3, y - 10, 7, 18, 3, color);
  spr.fillRect(x - 1, y - 6, 3, 13, color);
  spr.fillCircle(x, y + 9, 5, color);
  spr.fillCircle(x, y + 9, 2, HW_COLOR(255, 166, 40));
}

void EnvScreen::drawHumidityIcon(TFT_eSprite &spr, int x, int y,
                                 uint16_t color) {
  spr.fillTriangle(x, y - 12, x - 7, y + 3, x + 7, y + 3, color);
  spr.fillCircle(x, y + 3, 7, color);
  spr.fillRect(x - 2, y - 4, 4, 10, HW_COLOR(25, 124, 235));
  spr.drawLine(x + 2, y - 6, x + 5, y, COL_WHITE);
  spr.drawLine(x + 5, y, x + 3, y + 4, COL_WHITE);
}

void EnvScreen::drawPressureIcon(TFT_eSprite &spr, int x, int y,
                                 uint16_t color) {
  spr.drawCircle(x, y, 8, color);
  spr.drawCircle(x, y, 5, color);
  spr.fillRect(x - 1, y - 1, 3, 3, color);
  spr.drawLine(x, y, x + 4, y - 4, color);
  spr.drawFastHLine(x - 5, y + 5, 3, color);
  spr.drawFastHLine(x + 3, y + 5, 3, color);
  spr.drawFastVLine(x - 6, y - 1, 3, color);
  spr.drawFastVLine(x + 6, y - 1, 3, color);
}

void EnvScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw) {
    return;
  }

  createSprite();
  TFT_eSprite &spr = _screenSprite;
  drawBackground(spr);

  char timeBuf[8];
  sprintf(timeBuf, "%02d:%02d", _time.hour(), _time.minute());
  drawPixelText(spr, "BME280", 5, 4, 2, COL_WHITE);
  drawPixelText(spr, timeBuf, 86, 4, 2, COL_WHITE);

  if (!_data.valid) {
    drawPixelText(spr, "NO SENSOR", 64, 75, 2, HW_COLOR(255, 215, 84), true);
    spr.pushSprite(0, 0);
    _needsRedraw = false;
    return;
  }

  char tempBuf[16];
  char humBuf[16];
  char pressureBuf[16];
  sprintf(tempBuf, "%.1f", _data.temperature);
  sprintf(humBuf, "%.1f", _data.humidity);
  sprintf(pressureBuf, "%.1f", _data.pressure);

  spr.fillCircle(36, 55, 18, COL_PANEL);
  spr.fillRect(31, 73, 10, 3, COL_PANEL);
  spr.fillCircle(36, 94, 18, COL_PANEL);
  spr.fillRect(31, 112, 10, 3, COL_PANEL);
  spr.fillCircle(36, 133, 18, COL_PANEL);
  spr.fillRect(31, 151, 10, 3, COL_PANEL);

  for (int deg = 198; deg <= 348; deg += 3) {
    uint8_t mix = (uint8_t)(((deg - 198) * 255) / 150);
    uint16_t c = deg < 270 ? blendColor(COL_TEMP_A, COL_TEMP_B, mix * 2)
                           : blendColor(COL_TEMP_B, COL_TEMP_C,
                                        (uint8_t)((deg - 270) * 255 / 78));
    drawGaugeArc(spr, 36, 55, 21, deg, deg, c, 3);
  }
  drawGaugeArc(spr, 36, 94, 21, 184, 356, COL_HUM, 3);
  drawGaugeArc(spr, 36, 94, 21, 257, 356, COL_HUM_2, 3);
  drawGaugeArc(spr, 36, 133, 21, 184, 356, COL_PRESS, 3);
  drawGaugeArc(spr, 36, 133, 21, 257, 356, COL_PRESS_2, 3);

  drawThermometerIcon(spr, 36, 50, COL_TEMP_A);
  drawHumidityIcon(spr, 36, 90, COL_HUM_2);
  drawPressureIcon(spr, 36, 130, COL_PRESS_2);

  drawPixelText(spr, "TEMPERATURE", 65, 36, 1, COL_ORANGE);
  drawReading(spr, tempBuf, "C", 65, 49, 2, 2, COL_WHITE, COL_ORANGE, true);

  drawPixelText(spr, "HUMIDITY", 65, 75, 1, COL_HUM);
  drawReading(spr, humBuf, "%", 65, 88, 2, 2, COL_WHITE, COL_HUM, false);

  drawPixelText(spr, "PRESSURE", 65, 114, 1, COL_PRESS);
  drawReading(spr, pressureBuf, "hPa", 65, 127, 2, 1, COL_WHITE, COL_PRESS,
              false);

  spr.pushSprite(0, 0);
  _needsRedraw = false;
}

void EnvScreen::handleInput(const ControlState &state) {}
