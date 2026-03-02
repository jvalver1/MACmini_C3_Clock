#ifndef WEATHER_SCREEN_H
#define WEATHER_SCREEN_H

#include "Icons.h"
#include "ScreenManager.h"
#include "Zambretti.h"


class WeatherScreen : public Screen {
public:
  WeatherScreen(TFT_eSPI &tft, HardwareManager &hw, NetworkManager &net);
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  NetworkManager &_net;

  // Data
  float _apiTemp;
  String _apiDesc;
  WeatherForecast _localForecast;

  unsigned long _lastUpdate;
  bool _online;
  bool _needsRedraw;
};

#endif // WEATHER_SCREEN_H
