#include "WeatherScreen.h"
#include "TallFont.h"

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

void drawMidnightGradient(TFT_eSprite &spr) {
  for (int y = 0; y < 160; y++) {
    uint8_t amount = (uint8_t)((y * 255) / 159);
    uint16_t color =
        blendColor(HW_COLOR(0, 0, 0), HW_COLOR(4, 7, 14), amount);
    spr.drawFastHLine(0, y, 128, color);
  }
}

bool isSnowCode(int code) {
  return code == 71 || code == 73 || code == 75 || code == 77 ||
         code == 85 || code == 86;
}

bool isThunderCode(int code) { return code == 95 || code == 96 || code == 99; }

bool isRainCode(int code) {
  return (code >= 51 && code <= 67) || (code >= 80 && code <= 82) ||
         code == 85 || code == 86;
}

const char *windCardinal(int degrees) {
  static const char *dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int idx = ((degrees + 22) % 360) / 45;
  return dirs[idx];
}

void drawDrop(TFT_eSprite &spr, int x, int y, uint16_t color) {
  spr.fillTriangle(x, y, x - 4, y + 8, x + 4, y + 8, color);
  spr.fillCircle(x, y + 8, 4, color);
}

void drawSmallCloud(TFT_eSprite &spr, int x, int y, uint16_t color) {
  uint16_t shade = HW_COLOR(170, 170, 170);
  spr.fillCircle(x - 6, y + 4, 5, color);
  spr.fillCircle(x, y, 7, color);
  spr.fillCircle(x + 7, y + 4, 5, color);
  spr.fillRect(x - 10, y + 4, 20, 7, color);
  spr.drawFastHLine(x - 8, y + 10, 16, shade);
}

void drawRainMarks(TFT_eSprite &spr, int x, int y, int count) {
  uint16_t blue = HW_COLOR(70, 170, 255);
  for (int i = 0; i < count; i++) {
    int dx = (i - (count - 1) / 2) * 7;
    spr.drawLine(x + dx, y, x + dx - 2, y + 6, blue);
    spr.drawLine(x + dx + 1, y, x + dx - 1, y + 6, blue);
  }
}

void drawSnowMarks(TFT_eSprite &spr, int x, int y, int count) {
  uint16_t ice = HW_COLOR(150, 225, 255);
  for (int i = 0; i < count; i++) {
    int cx = x + (i - (count - 1) / 2) * 8;
    spr.drawFastHLine(cx - 3, y + 3, 7, ice);
    spr.drawFastVLine(cx, y, 7, ice);
    spr.drawLine(cx - 2, y + 1, cx + 2, y + 5, ice);
    spr.drawLine(cx + 2, y + 1, cx - 2, y + 5, ice);
  }
}

void drawWeatherIcon(TFT_eSprite &spr, int x, int y, int size, int code,
                     bool isDay) {
  uint16_t white = HW_COLOR(240, 240, 240);
  uint16_t yellow = HW_COLOR(255, 198, 18);
  uint16_t gray = HW_COLOR(180, 180, 180);
  int cloudY = y + size / 10;

  if (code == 0 && isDay) {
    int r = size / 5;
    spr.fillCircle(x, y, r, yellow);
    for (int i = 0; i < 360; i += 45) {
      float rad = i * DEG_TO_RAD;
      spr.drawLine(x + cos(rad) * (r + 3), y + sin(rad) * (r + 3),
                   x + cos(rad) * (r + 8), y + sin(rad) * (r + 8), yellow);
    }
    return;
  }

  if (code == 0 && !isDay) {
    spr.fillCircle(x - 3, y, size / 5, yellow);
    spr.fillCircle(x + 3, y - 2, size / 5, TFT_BLACK);
    spr.fillCircle(x + 13, y - 8, 1, white);
    spr.fillCircle(x + 18, y + 2, 1, white);
    return;
  }

  if ((code == 1 || code == 2) && isDay) {
    spr.fillCircle(x - size / 5, y - size / 8, size / 7, yellow);
    for (int i = 0; i < 360; i += 45) {
      float rad = i * DEG_TO_RAD;
      spr.drawLine(x - size / 5 + cos(rad) * (size / 7 + 2),
                   y - size / 8 + sin(rad) * (size / 7 + 2),
                   x - size / 5 + cos(rad) * (size / 7 + 6),
                   y - size / 8 + sin(rad) * (size / 7 + 6), yellow);
    }
  } else if ((code == 1 || code == 2) && !isDay) {
    spr.fillCircle(x - size / 4, y - size / 8, size / 6, yellow);
    spr.fillCircle(x - size / 5, y - size / 6, size / 6, TFT_BLACK);
  }

  drawSmallCloud(spr, x, cloudY, white);
  if (size > 28) {
    drawSmallCloud(spr, x + 10, cloudY + 3, gray);
  }

  if (code == 45 || code == 48) {
    spr.drawFastHLine(x - 18, y + size / 3, 36, gray);
    spr.drawFastHLine(x - 14, y + size / 3 + 5, 28, gray);
  }
  if (isRainCode(code)) {
    drawRainMarks(spr, x, y + size / 3, code >= 65 ? 3 : 2);
  }
  if (isSnowCode(code)) {
    drawSnowMarks(spr, x, y + size / 3, code == 75 ? 3 : 2);
  }
  if (isThunderCode(code)) {
    spr.fillTriangle(x + 6, y + 11, x + 1, y + 24, x + 8, y + 22,
                     yellow);
  }
}

void drawWindIcon(TFT_eSprite &spr, int x, int y, uint16_t color) {
  spr.drawFastHLine(x - 6, y - 3, 10, color);
  spr.drawFastHLine(x - 8, y + 2, 16, color);
  spr.drawFastHLine(x - 5, y + 7, 11, color);
  spr.drawLine(x + 3, y - 6, x + 8, y - 3, color);
}

void drawPressureIcon(TFT_eSprite &spr, int x, int y, uint16_t color) {
  spr.drawCircle(x, y, 8, color);
  spr.drawLine(x, y, x + 4, y - 5, color);
  spr.fillCircle(x, y, 2, color);
  spr.fillRect(x - 9, y + 4, 18, 7, TFT_BLACK);
}

void drawArrowIcon(TFT_eSprite &spr, int x, int y, uint16_t color) {
  spr.drawLine(x - 6, y + 6, x + 7, y - 7, color);
  spr.drawLine(x + 7, y - 7, x + 7, y + 1, color);
  spr.drawLine(x + 7, y - 7, x - 1, y - 7, color);
}

int tallNumberWidth(const char *str) {
  int total = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      total += tall_digit_widths[str[i] - '0'] + 1;
    }
  }
  return total > 0 ? total - 1 : 0;
}

void drawTallNumber(TFT_eSprite &spr, int x, int y, const char *str,
                    uint16_t color) {
  int currX = x;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] < '0' || str[i] > '9') {
      continue;
    }
    int idx = str[i] - '0';
    int w = tall_digit_widths[idx];
    int bytesPerRow = (w + 7) / 8;
    const uint8_t *bmp = tall_digits + tall_digit_offsets[idx];
    for (int py = 0; py < TALL_DIGIT_H; py++) {
      for (int px = 0; px < w; px++) {
        if (bmp[py * bytesPerRow + px / 8] & (1 << (7 - (px % 8)))) {
          spr.drawPixel(currX + px, y + py, color);
        }
      }
    }
    currX += w + 1;
  }
}
} // namespace

WeatherScreen::WeatherScreen(TFT_eSPI &tft, HardwareManager &hw,
                             NetworkManager &net)
    : _tft(tft), _hw(hw), _net(net), _lastUpdate(0), _online(false),
      _apiValid(false), _needsRedraw(true) {}

void WeatherScreen::update(unsigned long now) {
  if (now - _lastUpdate > 2000 || _lastUpdate == 0) { // Every 2 seconds
    // Read from thread-safe global status
    EnvironmentalData env = {0, 0, 0, false};
    bool online = false;
    WeatherData weather;
    bool apiValid = false;

    if (xStatusMutex != NULL && xSemaphoreTake(xStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      env = sysStatus.envData;
      online = sysStatus.wifiConnected;
      weather = sysStatus.weather;
      apiValid = sysStatus.weatherValid;
      xSemaphoreGive(xStatusMutex);
    }

    // Calculate Local Forecast (Zambretti)
    WeatherForecast localForecast =
        Zambretti::calculate(env.pressure, 0, 1); // Assuming steady for now

    if (localForecast != _localForecast || online != _online ||
        apiValid != _apiValid ||
        weather.temperature != _weather.temperature ||
        weather.description != _weather.description ||
        weather.precipitationProbability != _weather.precipitationProbability ||
        weather.cloudCover != _weather.cloudCover ||
        weather.pressure != _weather.pressure ||
        weather.windSpeed != _weather.windSpeed ||
        weather.windDirection != _weather.windDirection || _needsRedraw) {
      _localForecast = localForecast;
      _online = online;
      _apiValid = apiValid;
      _weather = weather;
      _needsRedraw = true;
    }

    _lastUpdate = now;
  }
}

void WeatherScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  TFT_eSprite spr = TFT_eSprite(&tft);
  spr.setColorDepth(16);
  spr.createSprite(128, 160);

  uint16_t white = HW_COLOR(245, 245, 245);
  uint16_t gray = HW_COLOR(96, 96, 96);
  uint16_t blue = HW_COLOR(105, 190, 255);
  uint16_t pale = HW_COLOR(205, 215, 225);

  drawMidnightGradient(spr);
  spr.drawFastHLine(5, 61, 118, gray);
  spr.drawFastHLine(5, 111, 118, gray);
  spr.drawFastVLine(63, 68, 36, gray);
  spr.drawFastVLine(63, 124, 34, gray);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(white);

  if (_online && _apiValid && _weather.valid) {
    char tempBuf[12];
    snprintf(tempBuf, sizeof(tempBuf), "%.0f", _weather.temperature);
    drawTallNumber(spr, 5, 5, tempBuf, white);
    int tempW = tallNumberWidth(tempBuf);
    spr.drawCircle(8 + tempW + 7, 14, 3, white);
    spr.drawString("C", 8 + tempW + 14, 13, 4);

    spr.setTextColor(blue);
    String desc = _weather.description;
    if (desc.length() > 15) {
      desc = desc.substring(0, 15);
    }
    spr.drawString(desc, 7, 49, 2);
    drawWeatherIcon(spr, 100, 31, 42, _weather.weatherCode, _weather.isDay);

    char value[18];
    int snowRisk = isSnowCode(_weather.weatherCode)
                       ? _weather.precipitationProbability
                       : 0;

    drawDrop(spr, 15, 70, blue);
    spr.setTextColor(white);
    snprintf(value, sizeof(value), "%d%%", _weather.precipitationProbability);
    spr.drawString(value, 27, 70, 2);

    drawSnowMarks(spr, 15, 86, 1);
    snprintf(value, sizeof(value), "%d%%", snowRisk);
    spr.drawString(value, 27, 87, 2);

    drawWindIcon(spr, 15, 103, white);
    snprintf(value, sizeof(value), "%.0f km/h", _weather.windSpeed);
    spr.drawString(value, 27, 102, 2);

    drawSmallCloud(spr, 79, 70, pale);
    snprintf(value, sizeof(value), "%d%%", _weather.cloudCover);
    spr.drawString(value, 92, 70, 2);

    drawPressureIcon(spr, 79, 88, white);
    snprintf(value, sizeof(value), "%.0f", _weather.pressure);
    spr.drawString(value, 92, 86, 1);
    spr.drawString("hPa", 92, 96, 1);

    drawArrowIcon(spr, 79, 104, white);
    spr.drawString(windCardinal(_weather.windDirection), 92, 102, 2);

    spr.setTextColor(blue);
    spr.drawString("Hourly [Next 2 hours]", 7, 116, 1);
    for (int i = 0; i < 2; i++) {
      int baseX = i == 0 ? 6 : 70;
      if (!_weather.hourly[i].valid) {
        continue;
      }
      snprintf(value, sizeof(value), "%02d:00", _weather.hourly[i].hour);
      spr.setTextColor(white);
      spr.drawString(value, baseX + 2, 126, 2);
      drawWeatherIcon(spr, baseX + 23, 143, 21, _weather.hourly[i].weatherCode,
                      _weather.isDay);
      snprintf(value, sizeof(value), "%.0fC", _weather.hourly[i].temperature);
      spr.drawString(value, baseX + 2, 143, 1);
      drawDrop(spr, baseX + 6, 151, blue);
      spr.setTextColor(blue);
      snprintf(value, sizeof(value), "%d%%",
               _weather.hourly[i].precipitationProbability);
      spr.drawString(value, baseX + 15, 149, 1);
      drawSmallCloud(spr, baseX + 31, 150, pale);
      spr.setTextColor(pale);
      snprintf(value, sizeof(value), "%d%%", _weather.hourly[i].cloudCover);
      spr.drawString(value, baseX + 39, 149, 1);
    }
  } else {
    drawWeatherIcon(spr, 64, 51, 48, 3, true);
    spr.setTextColor(HW_COLOR(255, 198, 18));
    spr.setTextDatum(TC_DATUM);
    spr.drawString("LOCAL TREND", 64, 82, 1);
    spr.setTextColor(white);
    String forecast = Zambretti::toString(_localForecast);
    if (forecast.length() > 18) {
      forecast = forecast.substring(0, 18);
    }
    spr.drawString(forecast, 64, 99, 1);
    spr.setTextColor(blue);
    spr.drawString("Open-Meteo offline", 64, 134, 1);
  }

  spr.pushSprite(0, 0);
  spr.deleteSprite();

  _needsRedraw = false;
}

void WeatherScreen::handleInput(const ControlState &state) {}
