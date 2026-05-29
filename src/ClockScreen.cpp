#include "ClockScreen.h"
#include "SleekFont.h"
#include "TallFont.h"
#include "SystemStatus.h"
#include <math.h>
#include <string.h>

// Helper to fix the ST7735 physical BGR hardware byte-swap
inline uint16_t HW_COLOR(uint8_t r, uint8_t g, uint8_t b) {
  return (((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

// Gradient theme
#define COL_GRAD_TIME HW_COLOR(255, 255, 0)   // Bright yellow
#define COL_GRAD_DATE HW_COLOR(255, 255, 255) // White

// Sleek Modern theme
#define COL_SLEEK_HOURS HW_COLOR(255, 255, 255)  // White
#define COL_SLEEK_MINS HW_COLOR(0, 255, 255)     // Cyan
#define COL_SLEEK_SECS HW_COLOR(255, 0, 0)       // Red
#define COL_SLEEK_DATE HW_COLOR(255, 255, 255)   // White
#define COL_SLEEK_BG HW_COLOR(18, 18, 18)        // Very dark grey
#define COL_SLEEK_BORDER HW_COLOR(150, 150, 150) // Light grey

// Layout (both themes)
#define Y_GRAD_TIME 80 // Center position
#define Y_GRAD_DATE 140

#define Y_SLEEK_HOUR 6
#define Y_SLEEK_MIN 54
#define Y_SLEEK_SEC 102
#define Y_SLEEK_DATE 157

#define X_CENTRE 64

// Fast integer square root algorithm
static uint32_t isqrt(uint32_t n) {
  uint32_t res = 0;
  uint32_t one = 1u << 30; // The second-to-top bit is set
  while (one > n) {
    one >>= 2;
  }
  while (one != 0) {
    if (n >= res + one) {
      n -= res + one;
      res = (res >> 1) + one;
    } else {
      res >>= 1;
    }
    one >>= 2;
  }
  return res;
}

// Helper function to draw the custom tall font to a Sprite for tear-free rendering
void drawTallStringSprite(TFT_eSprite &spr, int sprite_w, int sprite_h,
                          const char *str, uint16_t color) {
  int len = strlen(str);
  int total_w = 0;
  for (int i = 0; i < len; i++) {
    int idx = -1;
    if (str[i] >= '0' && str[i] <= '9')
      idx = str[i] - '0';
    else if (str[i] == ':')
      idx = 10;
    if (idx >= 0)
      total_w += tall_digit_widths[idx] + 2; // 2px letter spacing
  }
  if (total_w > 0)
    total_w -= 2;

  int start_x = (sprite_w - total_w) / 2;
  int start_y = (sprite_h - TALL_DIGIT_H) / 2;

  int curr_x = start_x;
  for (int i = 0; i < len; i++) {
    int idx = -1;
    if (str[i] >= '0' && str[i] <= '9')
      idx = str[i] - '0';
    else if (str[i] == ':')
      idx = 10;

    if (idx >= 0) {
      int w = tall_digit_widths[idx];
      int bytes_per_row = (w + 7) / 8;
      const uint8_t *bmp = tall_digits + tall_digit_offsets[idx];
      for (int y = 0; y < TALL_DIGIT_H; y++) {
        for (int x = 0; x < w; x++) {
          if (bmp[y * bytes_per_row + x / 8] & (1 << (7 - (x % 8)))) {
            spr.drawPixel(curr_x + x, start_y + y, color);
          }
        }
      }
      curr_x += w + 2;
    }
  }
}

// Helper function to draw the custom sleek font to a Sprite for tear-free rendering
void drawSleekStringSprite(TFT_eSprite &spr, int sprite_w, int sprite_h,
                           const char *str, uint16_t color) {
  int len = strlen(str);

  // Find max digit width to ensure monospaced alignment
  int max_digit_w = 0;
  for (int i = 0; i <= 9; i++) {
    if (sleek_digit_widths[i] > max_digit_w)
      max_digit_w = sleek_digit_widths[i];
  }
  int colon_w = sleek_digit_widths[10];

  int total_w = 0;
  for (int i = 0; i < len; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      total_w += max_digit_w + 2; // 2px letter spacing
    } else if (str[i] == ':' || str[i] == ' ') {
      total_w += colon_w + 2;
    }
  }
  if (total_w > 0)
    total_w -= 2;

  int start_x = (sprite_w - total_w) / 2;
  int start_y = (sprite_h - SLEEK_DIGIT_H) / 2;

  int curr_x = start_x;
  for (int i = 0; i < len; i++) {
    int idx = -1;
    bool is_digit = false;

    if (str[i] >= '0' && str[i] <= '9') {
      idx = str[i] - '0';
      is_digit = true;
    } else if (str[i] == ':') {
      idx = 10;
    }

    if (idx >= 0) {
      int w = sleek_digit_widths[idx];
      int bytes_per_row = (w + 7) / 8;
      const uint8_t *bmp = sleek_digits + sleek_digit_offsets[idx];

      int slot_w = is_digit ? max_digit_w : colon_w;
      int x_offset = (slot_w - w) / 2;

      for (int y = 0; y < SLEEK_DIGIT_H; y++) {
        for (int x = 0; x < w; x++) {
          if (bmp[y * bytes_per_row + x / 8] & (1 << (7 - (x % 8)))) {
            spr.drawPixel(curr_x + x_offset + x, start_y + y, color);
          }
        }
      }
      curr_x += slot_w + 2;
    } else if (str[i] == ' ') {
      curr_x += colon_w + 2;
    }
  }
}

// Draw the radial gradient portion into the sprite memory using LUT & mirroring
void drawGradientPart(TFT_eSprite &spr, int sprite_w, int sprite_h,
                      int screen_start_y, const uint16_t *gradColors) {
  // Center is at (64, 80) on the screen.
  // x goes from 0 to 127. dx = x - 64.
  // y goes from 0 to sprite_h - 1. screen_y = y + screen_start_y.
  // dy = screen_y - 80.
  // Since the gradient is perfectly symmetric around y=80 and x=64:
  // screen_y = 54 to 105. Middle of sprite is at y = 26.
  // We mirror y and 51 - y, and x and 127 - x.
  for (int y = 0; y < 26; y++) {
    int screen_y = y + screen_start_y;
    int dy = 80 - screen_y; // Absolute distance
    int dy2 = dy * dy;
    int y_mirror = 51 - y;
    for (int x = 0; x < 64; x++) {
      int dx = 64 - 1 - x;
      int d = isqrt(dx * dx + dy2);
      if (d > 120) d = 120;
      uint16_t color = gradColors[d];
      
      int x_mirror = 127 - x;
      
      spr.drawPixel(x, y, color);
      spr.drawPixel(x_mirror, y, color);
      spr.drawPixel(x, y_mirror, color);
      spr.drawPixel(x_mirror, y_mirror, color);
    }
  }
}

// ── Constructor & Destructor
// ───────────────────────────────────────────────────────────────
ClockScreen::ClockScreen(TFT_eSPI &tft, HardwareManager &hw)
    : _tft(tft), _hw(hw), _needsFullRedraw(true), _needsTimeRedraw(false),
      _lastSecond(-1), _lastMinute(-1), _theme(ClockTheme::GRADIENT),
      _timeSprite(&tft), _sprH(&tft), _sprM(&tft), _sprS(&tft), _spritesCreated(false) {
  
  // Precompute the gradient LUT: d = 0 to 120
  for (int d = 0; d <= 120; d++) {
    uint8_t r = 0;
    uint8_t g = (d * 20) / 120;  // 0 -> 20
    uint8_t b = (d * 150) / 120; // 0 -> 150
    _gradColors[d] = HW_COLOR(r, g, b);
  }
}

ClockScreen::~ClockScreen() {
  deleteSprites();
}

// ── Lifecycle
// ─────────────────────────────────────────────────────────────────
void ClockScreen::onEntry() {
  _needsFullRedraw = true;
  _lastSecond = -1;
  _lastMinute = -1;
  createSprites();
}

void ClockScreen::onExit() {
  deleteSprites();
}

// ── Persistent Sprite Allocations
// ─────────────────────────────────────────────────────────────────
void ClockScreen::createSprites() {
  if (_spritesCreated) return;
  
  Serial.println("[CLOCK] Allocating persistent sprites...");
  if (_theme == ClockTheme::GRADIENT) {
    _timeSprite.setColorDepth(16);
    _timeSprite.createSprite(128, TALL_DIGIT_H + 8);
  } else {
    int box_h = SLEEK_DIGIT_H + 4;
    _sprH.setColorDepth(16);
    _sprH.createSprite(124, box_h);
    _sprM.setColorDepth(16);
    _sprM.createSprite(124, box_h);
    _sprS.setColorDepth(16);
    _sprS.createSprite(124, box_h);
  }
  _spritesCreated = true;
}

void ClockScreen::deleteSprites() {
  if (!_spritesCreated) return;
  
  Serial.println("[CLOCK] Deleting persistent sprites...");
  if (_theme == ClockTheme::GRADIENT) {
    _timeSprite.deleteSprite();
  } else {
    _sprH.deleteSprite();
    _sprM.deleteSprite();
    _sprS.deleteSprite();
  }
  _spritesCreated = false;
}

// ── Update
// ────────────────────────────────────────────────────────────────────
void ClockScreen::update(unsigned long now) {
  // Read time from thread-safe global status
  DateTime current;
  if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    current = sysStatus.currentTime;
    xSemaphoreGive(xStatusMutex);
  } else {
    // Fallback if mutex unavailable
    current = _hw.getCurrentTime();
  }

  if (current.minute() != _lastMinute) {
    _lastTime = current;
    _lastMinute = current.minute();
    _lastSecond = current.second();

    if (_lastMinute == 0 && current.hour() == 0) {
      _needsFullRedraw = true;
    } else {
      _needsTimeRedraw = true;
    }
  } else if (current.second() != _lastSecond) {
    _lastTime = current;
    _lastSecond = current.second();
    _needsTimeRedraw = true;
  }
}

// ── Background helper (4-way quadrant mirroring + integer isqrt)
// ─────────────────────────────────────────────────────────
void ClockScreen::drawBackground(TFT_eSPI &tft) {
  if (_theme == ClockTheme::GRADIENT) {
    // Draw full screen radial gradient using 4-way quadrant mirroring around (64, 80)
    tft.startWrite(); // Fast SPI write block
    for (int dy = 0; dy < 80; dy++) {
      int dy2 = dy * dy;
      int y1 = 80 - 1 - dy; // Top half
      int y2 = 80 + dy;     // Bottom half
      for (int dx = 0; dx < 64; dx++) {
        int dx2 = dx * dx;
        int d = isqrt(dx2 + dy2);
        if (d > 120) d = 120;
        uint16_t color = _gradColors[d];
        
        int x1 = 64 - 1 - dx; // Left half
        int x2 = 64 + dx;     // Right half
        
        tft.drawPixel(x1, y1, color);
        tft.drawPixel(x2, y1, color);
        tft.drawPixel(x1, y2, color);
        tft.drawPixel(x2, y2, color);
      }
    }
    tft.endWrite();
  } else {
    // Sleek Modern: flat very-dark-grey
    tft.fillScreen(COL_SLEEK_BG);
    tft.drawRoundRect(0, 0, 128, 160, 8, COL_SLEEK_BORDER);
    tft.drawRoundRect(1, 1, 126, 158, 7, COL_SLEEK_BORDER);
  }
}

// ── Draw
// ──────────────────────────────────────────────────────────────────────
void ClockScreen::draw(TFT_eSPI &tft) {
  if (_needsFullRedraw) {
    drawBackground(tft);

    char dateBuf[16];
    sprintf(dateBuf, "%02d/%02d/%04d", _lastTime.day(), _lastTime.month(),
            _lastTime.year());

    if (_theme == ClockTheme::GRADIENT) {
      tft.setTextDatum(BC_DATUM);
      tft.setTextColor(COL_GRAD_DATE);
      tft.drawString(dateBuf, X_CENTRE, Y_GRAD_DATE, 2);

      char timeBuf[16];
      sprintf(timeBuf, "%02d:%02d:%02d", _lastTime.hour(), _lastTime.minute(),
              _lastTime.second());

      int box_h = TALL_DIGIT_H + 8;
      int start_y = Y_GRAD_TIME - box_h / 2;

      // Draw gradient and text into persistent sprite
      _timeSprite.fillSprite(TFT_BLACK);
      drawGradientPart(_timeSprite, 128, box_h, start_y, _gradColors);
      drawTallStringSprite(_timeSprite, 128, box_h, timeBuf, COL_GRAD_TIME);
      _timeSprite.pushSprite(0, start_y);

    } else { // SLEEK
      uint16_t bgCol = COL_SLEEK_BG;
      int box_h = SLEEK_DIGIT_H + 4;

      char bufH[8], bufM[8], bufS[8];
      sprintf(bufH, "%02d:", _lastTime.hour());
      sprintf(bufM, "%02d:", _lastTime.minute());
      sprintf(bufS, "%02d ", _lastTime.second());

      _sprH.fillSprite(bgCol);
      drawSleekStringSprite(_sprH, 124, box_h, bufH, COL_SLEEK_HOURS);
      _sprH.pushSprite(2, Y_SLEEK_HOUR);

      _sprM.fillSprite(bgCol);
      drawSleekStringSprite(_sprM, 124, box_h, bufM, COL_SLEEK_MINS);
      _sprM.pushSprite(2, Y_SLEEK_MIN);

      _sprS.fillSprite(bgCol);
      drawSleekStringSprite(_sprS, 124, box_h, bufS, COL_SLEEK_SECS);
      _sprS.pushSprite(2, Y_SLEEK_SEC);
    }

    _needsFullRedraw = false;
    _needsTimeRedraw = false;

  } else if (_needsTimeRedraw) {
    if (_theme == ClockTheme::GRADIENT) {
      char timeBuf[16];
      sprintf(timeBuf, "%02d:%02d:%02d", _lastTime.hour(), _lastTime.minute(),
              _lastTime.second());

      int box_h = TALL_DIGIT_H + 8;
      int start_y = Y_GRAD_TIME - box_h / 2;

      _timeSprite.fillSprite(TFT_BLACK);
      drawGradientPart(_timeSprite, 128, box_h, start_y, _gradColors);
      drawTallStringSprite(_timeSprite, 128, box_h, timeBuf, COL_GRAD_TIME);
      _timeSprite.pushSprite(0, start_y);

    } else { // SLEEK
      uint16_t bgCol = COL_SLEEK_BG;
      int box_h = SLEEK_DIGIT_H + 4;

      char bufH[8], bufM[8], bufS[8];
      sprintf(bufH, "%02d:", _lastTime.hour());
      sprintf(bufM, "%02d:", _lastTime.minute());
      sprintf(bufS, "%02d ", _lastTime.second());

      _sprH.fillSprite(bgCol);
      drawSleekStringSprite(_sprH, 124, box_h, bufH, COL_SLEEK_HOURS);
      _sprH.pushSprite(2, Y_SLEEK_HOUR);

      _sprM.fillSprite(bgCol);
      drawSleekStringSprite(_sprM, 124, box_h, bufM, COL_SLEEK_MINS);
      _sprM.pushSprite(2, Y_SLEEK_MIN);

      _sprS.fillSprite(bgCol);
      drawSleekStringSprite(_sprS, 124, box_h, bufS, COL_SLEEK_SECS);
      _sprS.pushSprite(2, Y_SLEEK_SEC);
    }
    _needsTimeRedraw = false;
  }
}

// ── Input
// ─────────────────────────────────────────────────────────────────────
void ClockScreen::handleInput(const ControlState &state) {
  if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
    // Delete sprites of current theme
    deleteSprites();
    // Swap theme
    _theme = (_theme == ClockTheme::GRADIENT) ? ClockTheme::SLEEK : ClockTheme::GRADIENT;
    // Reallocate sprites for the new theme
    createSprites();
    
    _needsFullRedraw = true;
    Serial.printf("[CLOCK] Theme toggled -> %s\n",
                  (_theme == ClockTheme::GRADIENT) ? "GRADIENT" : "SLEEK");
  }
}
