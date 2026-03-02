#include "ClockScreen.h"
#include "SleekFont.h"
#include "TallFont.h"
#include <math.h>
#include <string.h>

// ── Colour palette
// ────────────────────────────────────────────────────────────
// Helper to fix the ST7735 physical BGR hardware byte-swap
inline uint16_t HW_COLOR(uint8_t r, uint8_t g, uint8_t b) {
  return (((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

// Gradient theme
#define COL_GRAD_TIME HW_COLOR(255, 255, 0)   // Bright yellow
#define COL_GRAD_DATE HW_COLOR(255, 255, 255) // White

// Sleek Modern theme
#define COL_SLEEK_HOURS HW_COLOR(255, 255, 255) // White
#define COL_SLEEK_MINS HW_COLOR(0, 255, 255)    // Cyan
#define COL_SLEEK_SECS HW_COLOR(255, 0, 0)      // Red
#define COL_SLEEK_DATE HW_COLOR(255, 255, 255)  // White
#define COL_SLEEK_BG HW_COLOR(18, 18, 18)       // Very dark grey

// ── Layout (both themes)
// ──────────────────────────────────────────────────────
#define Y_GRAD_TIME 80 // Center position
#define Y_GRAD_DATE 140

#define Y_SLEEK_HOUR 4
#define Y_SLEEK_MIN 52
#define Y_SLEEK_SEC 100
#define Y_SLEEK_DATE 155

#define X_CENTRE 64

// Helper function to draw the custom tall font to a Sprite for tear-free
// rendering
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

// Helper function to draw the custom sleek font to a Sprite for tear-free
// rendering
void drawSleekStringSprite(TFT_eSprite &spr, int sprite_w, int sprite_h,
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
      total_w += sleek_digit_widths[idx] + 2; // 2px letter spacing
    else if (str[i] == ' ')
      total_w += sleek_digit_widths[10] + 2;
  }
  if (total_w > 0)
    total_w -= 2;

  int start_x = (sprite_w - total_w) / 2;
  int start_y = (sprite_h - SLEEK_DIGIT_H) / 2;

  int curr_x = start_x;
  for (int i = 0; i < len; i++) {
    int idx = -1;
    if (str[i] >= '0' && str[i] <= '9')
      idx = str[i] - '0';
    else if (str[i] == ':')
      idx = 10;

    if (idx >= 0) {
      int w = sleek_digit_widths[idx];
      int bytes_per_row = (w + 7) / 8;
      const uint8_t *bmp = sleek_digits + sleek_digit_offsets[idx];
      for (int y = 0; y < SLEEK_DIGIT_H; y++) {
        for (int x = 0; x < w; x++) {
          if (bmp[y * bytes_per_row + x / 8] & (1 << (7 - (x % 8)))) {
            spr.drawPixel(curr_x + x, start_y + y, color);
          }
        }
      }
      curr_x += w + 2;
    } else if (str[i] == ' ') {
      curr_x += sleek_digit_widths[10] + 2;
    }
  }
}
// Draw the radial gradient portion into the sprite memory
void drawGradientPart(TFT_eSprite &spr, int sprite_w, int sprite_h,
                      int screen_start_y) {
  for (int y = 0; y < sprite_h; y++) {
    int screen_y = y + screen_start_y;
    int dy = screen_y - 80;
    int dy2 = dy * dy;
    for (int x = 0; x < sprite_w; x++) {
      int dx = x - 64;
      int d = sqrt(dx * dx + dy2);

      if (d > 120)
        d = 120;

      uint8_t r = 0;
      uint8_t g = (d * 20) / 120;
      uint8_t b = (d * 150) / 120;

      spr.drawPixel(x, y, HW_COLOR(r, g, b));
    }
  }
}

// ── Constructor
// ───────────────────────────────────────────────────────────────
ClockScreen::ClockScreen(TFT_eSPI &tft, HardwareManager &hw)
    : _tft(tft), _hw(hw), _needsFullRedraw(true), _needsTimeRedraw(false),
      _lastSecond(-1), _lastMinute(-1), _theme(ClockTheme::GRADIENT) {}

// ── Lifecycle
// ─────────────────────────────────────────────────────────────────
void ClockScreen::onEntry() {
  _needsFullRedraw = true;
  _lastSecond = -1;
  _lastMinute = -1;
}

// ── Update
// ────────────────────────────────────────────────────────────────────
void ClockScreen::update(unsigned long now) {
  DateTime current = _hw.getCurrentTime();
  if (current.minute() != _lastMinute) {
    _lastTime = current;
    _lastMinute = current.minute();
    _lastSecond = current.second();

    // We only need a full screen redraw (to update the date and whole
    // background) when the day changes, or upon initial entry. For minute
    // changes, updating the time sprite is enough!
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

// ── Background helper
// ─────────────────────────────────────────────────────────
void ClockScreen::drawBackground(TFT_eSPI &tft) {
  if (_theme == ClockTheme::GRADIENT) {
    // Draw full screen radial gradient manually (only done once per minute)
    for (int y = 0; y < 160; y++) {
      int dy = y - 80;
      int dy2 = dy * dy;
      for (int x = 0; x < 128; x++) {
        int dx = x - 64;
        int d = sqrt(dx * dx + dy2);

        // Let's stretch the gradient further to the corners
        if (d > 120)
          d = 120;

        // At center (d=0) we want Dark Navy (e.g. 0, 0, 20)
        // At edges (d=120) we want a richer, almost black blue (e.g. 0, 0, 4)
        // Actually, user wants Navy Blue on the OUTSIDE, and Black on the
        // INSIDE. d=0 -> Black (0, 0, 0) d=120 -> Navy Blue (0, 20, 80)

        uint8_t r = 0;
        uint8_t g = (d * 20) / 120;  // 0 -> 20
        uint8_t b = (d * 150) / 120; // 0 -> 150

        tft.drawPixel(x, y, HW_COLOR(r, g, b));
      }
    }
  } else {
    // Sleek Modern: flat very-dark-grey
    tft.fillScreen(0x1082); // ~#121212 in RGB565
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

      // Create a sprite to draw the time without flickering
      int box_h = TALL_DIGIT_H + 8;
      int start_y = Y_GRAD_TIME - box_h / 2;

      TFT_eSprite spr = TFT_eSprite(&tft);
      spr.setColorDepth(16);
      spr.createSprite(128, box_h);

      drawGradientPart(spr, 128, box_h, start_y);
      drawTallStringSprite(spr, 128, box_h, timeBuf, COL_GRAD_TIME);

      spr.pushSprite(0, start_y);
      spr.deleteSprite();

    } else { // SLEEK
      uint16_t bgCol = COL_SLEEK_BG;

      // Using sleek sprite to avoid flicker and use proportional font
      int box_h = SLEEK_DIGIT_H + 4;

      char bufH[8], bufM[8], bufS[8];
      sprintf(bufH, "%02d:", _lastTime.hour());
      sprintf(bufM, "%02d:", _lastTime.minute());
      sprintf(bufS, "%02d ", _lastTime.second());

      TFT_eSprite sprH = TFT_eSprite(&tft);
      sprH.setColorDepth(16);
      sprH.createSprite(128, box_h);
      sprH.fillSprite(bgCol);
      drawSleekStringSprite(sprH, 128, box_h, bufH, COL_SLEEK_HOURS);
      sprH.pushSprite(0, Y_SLEEK_HOUR);
      sprH.deleteSprite();

      TFT_eSprite sprM = TFT_eSprite(&tft);
      sprM.setColorDepth(16);
      sprM.createSprite(128, box_h);
      sprM.fillSprite(bgCol);
      drawSleekStringSprite(sprM, 128, box_h, bufM, COL_SLEEK_MINS);
      sprM.pushSprite(0, Y_SLEEK_MIN);
      sprM.deleteSprite();

      TFT_eSprite sprS = TFT_eSprite(&tft);
      sprS.setColorDepth(16);
      sprS.createSprite(128, box_h);
      sprS.fillSprite(bgCol);
      drawSleekStringSprite(sprS, 128, box_h, bufS, COL_SLEEK_SECS);
      sprS.pushSprite(0, Y_SLEEK_SEC);
      sprS.deleteSprite();
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

      TFT_eSprite spr = TFT_eSprite(&tft);
      spr.setColorDepth(16);
      spr.createSprite(128, box_h);

      drawGradientPart(spr, 128, box_h, start_y);
      drawTallStringSprite(spr, 128, box_h, timeBuf, COL_GRAD_TIME);

      spr.pushSprite(0, start_y);
      spr.deleteSprite();

    } else { // SLEEK
      uint16_t bgCol = COL_SLEEK_BG;
      int box_h = SLEEK_DIGIT_H + 4;
      TFT_eSprite sprS = TFT_eSprite(&tft);
      sprS.setColorDepth(16);
      sprS.createSprite(128, box_h);
      sprS.fillSprite(bgCol);

      char bufS[8];
      sprintf(bufS, "%02d ", _lastTime.second());

      drawSleekStringSprite(sprS, 128, box_h, bufS, COL_SLEEK_SECS);
      sprS.pushSprite(0, Y_SLEEK_SEC);
      sprS.deleteSprite();
    }
    _needsTimeRedraw = false;
  }
}

// ── Input
// ─────────────────────────────────────────────────────────────────────
void ClockScreen::handleInput(const ControlState &state) {
  if (state.joyCenter == ButtonEvent::SHORT_PRESS) {
    _theme = (_theme == ClockTheme::GRADIENT) ? ClockTheme::SLEEK
                                              : ClockTheme::GRADIENT;
    _needsFullRedraw = true;
    Serial.printf("[CLOCK] Theme toggled -> %s\n",
                  (_theme == ClockTheme::GRADIENT) ? "GRADIENT" : "SLEEK");
  }
}
