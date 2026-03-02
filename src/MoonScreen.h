#ifndef MOON_SCREEN_H
#define MOON_SCREEN_H

#include "Icons.h"
#include "MoonPhase.h"
#include "ScreenManager.h"

class MoonScreen : public Screen {
public:
  MoonScreen(TFT_eSPI &tft, HardwareManager &hw);
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  MoonData _moon;
  float _lastAge;
  bool _needsRedraw;
};

#endif // MOON_SCREEN_H
