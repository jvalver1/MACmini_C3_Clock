#include "WeatherScreen.h"

WeatherScreen::WeatherScreen(TFT_eSPI &tft, HardwareManager &hw,
                             NetworkManager &net)
    : _tft(tft), _hw(hw), _net(net), _lastUpdate(0), _online(false),
      _needsRedraw(true) {}

void WeatherScreen::update(unsigned long now) {
  if (now - _lastUpdate > 10 * 60 * 1000 || _lastUpdate == 0) { // Every 10 mins
    _online = (WiFi.status() == WL_CONNECTED);

    // Calculate Local Forecast (Zambretti)
    EnvironmentalData env = _hw.getEnvironmentalData();
    _localForecast =
        Zambretti::calculate(env.pressure, 0, 1); // Assuming steady for now

    if (_online) {
      // Attempt API fetch
      // _net.fetchWeather(...)
    }

    _lastUpdate = now;
    _needsRedraw = true;
  }
}

void WeatherScreen::draw(TFT_eSPI &tft) {
  if (!_needsRedraw)
    return;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("WEATHER", 64, 10, 2);

  if (_online) {
    Icons::drawSun(tft, 64, 70, 60, 0xFD00); // Sample Online Icon
    tft.setTextColor(0xC618, TFT_BLACK);
    tft.drawString("ONLINE FORECAST", 64, 110, 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Clear Skies", 64, 125, 2);
  } else {
    // Draw Local Trend (Zambretti)
    Icons::drawCloud(tft, 64, 70, 60, 0x7BEF);
    tft.setTextColor(0xFD00, TFT_BLACK); // Alert color
    tft.drawString("LOCAL TREND", 64, 110, 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(Zambretti::toString(_localForecast), 64, 125, 2);
  }

  // Current City from config (not hardcoded)
  ConfigData cfg = _net.loadConfig();
  tft.setTextColor(0xBDD7, TFT_BLACK); // Gray-Blue
  tft.drawString(cfg.cityName.c_str(), 64, 150, 1);

  _needsRedraw = false;
}

void WeatherScreen::handleInput(const ControlState &state) {}
