#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include "ScreenManager.h"
#include <RTClib.h>

enum class ClockTheme { GRADIENT = 0, SLEEK };

class ClockScreen : public Screen {
public:
  ClockScreen(TFT_eSPI &tft, HardwareManager &hw);
  void onEntry() override;
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  void drawBackground(TFT_eSPI &tft);

  TFT_eSPI &_tft;
  HardwareManager &_hw;
  DateTime _lastTime;
  bool _needsFullRedraw;
  bool _needsTimeRedraw;
  int _lastSecond;
  int _lastMinute;
  ClockTheme _theme;
};

#endif // CLOCK_SCREEN_H
