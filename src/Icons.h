#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>

class Icons {
public:
  static void drawSun(TFT_eSPI &tft, int x, int y, int size, uint16_t color) {
    int r = size / 3;
    tft.drawCircle(x, y, r, color);
    // Rays
    for (int i = 0; i < 360; i += 45) {
      float rad = i * DEG_TO_RAD;
      int x1 = x + cos(rad) * (r + 2);
      int y1 = y + sin(rad) * (r + 2);
      int x2 = x + cos(rad) * (size / 2);
      int y2 = y + sin(rad) * (size / 2);
      tft.drawLine(x1, y1, x2, y2, color);
    }
  }

  static void drawCloud(TFT_eSPI &tft, int x, int y, int size, uint16_t color) {
    int r = size / 4;
    tft.fillCircle(x - r, y, r, color);
    tft.fillCircle(x, y - r / 2, r, color);
    tft.fillCircle(x + r, y, r, color);
    tft.fillRect(x - r, y, r * 2, r, color);
  }

  static void drawRain(TFT_eSPI &tft, int x, int y, int size, uint16_t color) {
    drawCloud(tft, x, y - 2, size, color);
    // Drops
    for (int i = -1; i <= 1; i++) {
      tft.drawLine(x + i * 6, y + 4, x + i * 6 - 2, y + 8, color);
    }
  }

  // Draw the moon to represent the current phase.
  // illumination : 0.0 (new) .. 1.0 (full)
  // waxing       : true = lit side on right, false = lit side on left
  static void drawMoon(TFT_eSPI &tft, int x, int y, int size, uint16_t color,
                       float illumination, bool waxing) {
    int r = size / 2;

    // Fill the whole moon in black first (dark sky behind the sphere)
    tft.fillCircle(x, y, r, TFT_BLACK);

    if (illumination < 0.01f)
      return; // New Moon – nothing visible

    // Terminator ellipse horizontal semi-axis:
    // 0% illum -> terminator at centre (ta = 0) showing nothing lit
    // 50% illum -> terminator at r (full half lit)
    // 100% illum -> Full Moon (we just fill the circle)
    if (illumination > 0.99f) {
      tft.fillCircle(x, y, r, color);
      return;
    }

    // Map illumination to terminator semi-axis & which side is dark
    // illum 0..0.5 : waxing phases, lit sliver growing on one side
    // illum 0.5..1 : gibbous phases, dark sliver shrinking
    float ta; // terminator semi-axis (>=0 means convex toward dark side)
    bool terminatorBulgesLeft; // direction the terminator bulges

    if (illumination <= 0.5f) {
      // Crescent: lit strip, terminator bulges into the lit side
      ta = (float)r * (1.0f - 2.0f * illumination); // r..0
      terminatorBulgesLeft = waxing; // for waxing, dark is on left
    } else {
      // Gibbous: dark strip, terminator bulges into the dark side
      ta = (float)r * (2.0f * illumination - 1.0f); // 0..r
      terminatorBulgesLeft = !waxing; // for waxing, dark strip is on left
    }

    // Scan line by line inside the moon circle
    for (int dy = -r; dy <= r; dy++) {
      // Half-width of the moon circle at this row
      float limb = sqrtf((float)(r * r - dy * dy));
      if (limb < 0.5f)
        continue;

      // Half-width of the terminator ellipse at this row
      float term = sqrtf(
          fmaxf(0.0f, ta * ta - (float)(dy * dy) * ta * ta / ((float)r * r)));

      int xLeft, xRight;
      if (illumination <= 0.5f) {
        // Crescent: lit only between terminator and limb on the lit side
        if (waxing) {
          // Lit side = right; terminator arc faces left into the lit zone
          if (terminatorBulgesLeft) {
            xLeft = x + (int)(-term);
            xRight = x + (int)(limb);
          } else {
            xLeft = x + (int)(-limb);
            xRight = x + (int)(term);
          }
        } else {
          // Lit side = left
          if (terminatorBulgesLeft) {
            xLeft = x + (int)(-term);
            xRight = x + (int)(limb);
          } else {
            xLeft = x + (int)(-limb);
            xRight = x + (int)(term);
          }
        }
      } else {
        // Gibbous: full limb lit, minus a dark crescent on the dark side
        if (waxing) {
          // Dark side = left
          xLeft = x + (int)(-limb);
          xRight = x + (int)(limb);
          // Erase dark strip on the left
          if (term > 0) {
            tft.drawFastHLine(xLeft, y + dy, (int)(limb - term), TFT_BLACK);
            xLeft = x - (int)term;
          }
        } else {
          // Dark side = right
          xLeft = x + (int)(-limb);
          xRight = x + (int)(limb);
          if (term > 0) {
            tft.drawFastHLine(x + (int)term, y + dy, (int)(limb - term),
                              TFT_BLACK);
            xRight = x + (int)term;
          }
        }
      }

      if (xRight > xLeft)
        tft.drawFastHLine(xLeft, y + dy, xRight - xLeft, color);
    }
  }

  // Legacy stub kept for any other callers (draws a simple crescent)
  static void drawMoon(TFT_eSPI &tft, int x, int y, int size, uint16_t color) {
    drawMoon(tft, x, y, size, color, 0.35f, true);
  }

  static void drawThermometer(TFT_eSPI &tft, int x, int y, int size,
                              uint16_t color) {
    tft.drawCircle(x, y + size / 3, size / 4, color);
    tft.drawRoundRect(x - 2, y - size / 2, 4, size - 4, 2, color);
  }
};

#endif // ICONS_H
