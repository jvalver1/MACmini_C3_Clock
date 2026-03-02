#ifndef ENV_SCREEN_H
#define ENV_SCREEN_H

#include "Icons.h"
#include "ScreenManager.h"


class EnvScreen : public Screen {
public:
  EnvScreen(TFT_eSPI &tft, HardwareManager &hw);
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  EnvironmentalData _data;
  unsigned long _lastUpdate;
  bool _needsRedraw;
};

#endif // ENV_SCREEN_H
